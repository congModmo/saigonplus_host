
#define __DEBUG__ 3
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "log_sys.h"
#include "task.h"
#include "core_mqtt.h"
#include "app_lte/lteTask.h"
#include "app_config.h"
#include "mqttTask.h"
#include "app_main/app_info.h"
#include "iwdg.h"
#include "app_main/app_main.h"

#define STATUS_TOPIC_PREFIX								"/status/v3/"
#define CMD_TOPIC_PREFIX 								"/cmd/v3/"
#define ACK_TOPIC_PREFIX 								"/ack/v3/"
#define TEST_TOPIC_PREFIX								"/test/v3/"
#define CLIENT_IDENTIFIER_PREFIX                        "saigonplus_v2_"

static bool mqtt_ready=false;

bool mqtt_is_ready(){
	return mqtt_ready;
}

#define MQTT_TOPIC_COUNT                            ( 3 )
#define MQTT_MESSAGE                                "Hello World!"
#define MQTT_PROCESS_LOOP_TIMEOUT_MS                ( 5000U )
#define MQTT_KEEP_ALIVE_TIMEOUT_SECONDS             ( 60U )
#define MQTT_DELAY_BETWEEN_PUBLISHES_TICKS          ( pdMS_TO_TICKS( 2000U ) )
#define MQTT_TRANSPORT_SEND_RECV_TIMEOUT_MS         ( 500U )
#define MILLISECONDS_PER_SECOND                     ( 1000U )
#define MILLISECONDS_PER_TICK                       ( MILLISECONDS_PER_SECOND / configTICK_RATE_HZ )
#define MQTT_CONNACK_RECV_TIMEOUT_MS                ( 2000U )
#define MQTT_NETWORK_BUFFER_SIZE					512

static BaseType_t prvCreateMQTTConnectionWithBroker( MQTTContext_t * pxMQTTContext,
                                                     NetworkContext_t * pxNetworkContext );
static void prvUpdateSubAckStatus( MQTTPacketInfo_t * pxPacketInfo );
static BaseType_t prvMQTTSubscribe( char * topic );
static BaseType_t prvMQTTPublishToTopic( char *topic, uint8_t *data, uint16_t len, int QoS);
static uint32_t prvGetTimeMs( void );
static void prvMQTTProcessResponse( MQTTPacketInfo_t * pxIncomingPacket,
                                    uint16_t usPacketId );
static void prvMQTTProcessIncomingPublish( MQTTPublishInfo_t * pxPublishInfo );
static void prvEventCallback( MQTTContext_t * pxMQTTContext,
                              MQTTPacketInfo_t * pxPacketInfo,
                              MQTTDeserializedInfo_t * pxDeserializedInfo );
static MQTTStatus_t prvWaitForPacket( MQTTContext_t * pxMQTTContext,
                                      uint16_t usPacketType );

static uint8_t ucSharedBuffer[ MQTT_NETWORK_BUFFER_SIZE ];
static uint32_t ulGlobalEntryTimeMs;
static uint16_t usPublishPacketIdentifier;
static uint16_t usSubscribePacketIdentifier;
static uint16_t usUnsubscribePacketIdentifier;
static uint16_t usPacketTypeReceived = 0U;

//v2
static char status_topic[64];
static char cmd_topic[64];
static char ack_topic[64];
static char client_id[64];

#ifdef JIGTEST
#include "jigtest.h"
static char test_topic[64];
static char test_message[]="hello from Modmo, wishing you have a nice day";
static int test_count;
static int test_success;
const int test_num=5;
static uint32_t test_tick;
__IO bool mqtt_test_done=false;
__IO bool mqtt_test_result=false;
#endif

typedef struct topicFilterContext
{
    const char * pcTopicFilter;
    MQTTSubAckStatus_t xSubAckStatus;
} topicFilterContext_t;

typedef struct{
	MQTTContext_t MQTTContext ;
	NetworkContext_t NetworkContext;
	topicFilterContext_t topic;
	lara_r2_socket_t socket;
	int publish_failed;
//	mqtt_msg_callback_t message_cb;
}mqtt_t;

static mqtt_t mqtt={0};
//extern osEventFlagsId_t NetworkEventHandle;

static MQTTFixedBuffer_t xBuffer =
{
    ucSharedBuffer,
	MQTT_NETWORK_BUFFER_SIZE
};

static void mqtt_publish(char *topic, uint8_t *data, size_t len, int qos){
	if(pdPASS==prvMQTTPublishToTopic(topic, data, len, qos)){
		mqtt.publish_failed=0;
		info("Publish done\n");
	}
	else{
		mqtt.publish_failed++;
		error("Publish failed\n");
	}
}
/*-----------------------------------------------------------*/
int32_t TransportRecv( NetworkContext_t * pNetworkContext, void * pBuffer, size_t bytesToRecv ){
	return lara_r2_socket_read(pNetworkContext->socket, pBuffer, bytesToRecv);
}

int32_t TransportSend( NetworkContext_t * pNetworkContext, const void * pBuffer, size_t bytesToSend ){
	return lara_r2_socket_send(pNetworkContext->socket, pBuffer, bytesToSend);
}

/*-----------------------------------------------------------*/

static BaseType_t prvCreateMQTTConnectionWithBroker( MQTTContext_t * pxMQTTContext,
                                                     NetworkContext_t * pxNetworkContext )
{
    MQTTStatus_t xResult;
    MQTTConnectInfo_t xConnectInfo={0};
    bool xSessionPresent;
    TransportInterface_t xTransport;
    BaseType_t xStatus = pdFAIL;
    xTransport.pNetworkContext = pxNetworkContext;
    xTransport.send = TransportSend;
    xTransport.recv = TransportRecv;
    xResult = MQTT_Init( pxMQTTContext, &xTransport, prvGetTimeMs, prvEventCallback, &xBuffer );
    configASSERT( xResult == MQTTSuccess );
    xConnectInfo.cleanSession = true;
    xConnectInfo.pClientIdentifier = client_id;
    xConnectInfo.clientIdentifierLength = ( uint16_t ) strlen( client_id );
    xConnectInfo.keepAliveSeconds = MQTT_KEEP_ALIVE_TIMEOUT_SECONDS;
    xResult = MQTT_Connect( pxMQTTContext,
                            &xConnectInfo,
                            NULL,
                            MQTT_CONNACK_RECV_TIMEOUT_MS,
                            &xSessionPresent );

    if( xResult != MQTTSuccess )
    {
        error(  "Failed to establish MQTT connection: Server=%s, MQTTStatus=%s\n",
                    factory_config->broker.endpoint, MQTT_Status_strerror( xResult ))  ;
    }
    else
    {
        /* Successfully established and MQTT connection with the broker. */
        info ( "An MQTT connection is established with %s.\n", factory_config->broker.endpoint ) ;
        xStatus = pdPASS;
    }

    return xStatus;
}
/*-----------------------------------------------------------*/

static void prvUpdateSubAckStatus( MQTTPacketInfo_t * pxPacketInfo )
{
    MQTTStatus_t xResult = MQTTSuccess;
    uint8_t * pucPayload = NULL;
    size_t ulSize = 0;

    xResult = MQTT_GetSubAckStatusCodes( pxPacketInfo, &pucPayload, &ulSize );

    /* MQTT_GetSubAckStatusCodes always returns success if called with packet info
     * from the event callback and non-NULL parameters. */
    configASSERT( xResult == MQTTSuccess );
    mqtt.topic.xSubAckStatus= pucPayload[0];
}
/*-----------------------------------------------------------*/

static BaseType_t prvMQTTSubscribe( char *topic)
{
    MQTTStatus_t xResult = MQTTSuccess;
    MQTTSubscribeInfo_t xMQTTSubscription ={0};
    BaseType_t xFailedSubscribeToTopic = pdFALSE;
    int retry=3;
    BaseType_t xStatus = pdFAIL;
    xMQTTSubscription.qos = MQTTQoS0;
    xMQTTSubscription.pTopicFilter = topic;
    xMQTTSubscription.topicFilterLength = ( uint16_t ) strlen( topic );
    mqtt.topic.pcTopicFilter=topic;
    mqtt.topic.xSubAckStatus=MQTTSubAckFailure;
    do
    {
        info ( "Attempt to subscribe to the MQTT topic %s.\n", topic ) ;
        usSubscribePacketIdentifier=MQTT_GetPacketId( &mqtt.MQTTContext);
        xResult = MQTT_Subscribe( &mqtt.MQTTContext,
                                  &xMQTTSubscription,
                                  1,
								  usSubscribePacketIdentifier);

        if( xResult != MQTTSuccess )
        {
            error(  "Failed to SUBSCRIBE to MQTT  Error=%s\n", MQTT_Status_strerror( xResult ) ) ;
        }
        else
        {
            xStatus = pdPASS;
            info ( "SUBSCRIBE sent for topic to broker.\n" ) ;
            xResult = prvWaitForPacket( &mqtt.MQTTContext, MQTT_PACKET_TYPE_SUBACK );

            if( xResult != MQTTSuccess )
            {
                xStatus = pdFAIL;
            }
        }

        if( xStatus == pdPASS )
        {
            /* Reset flag before checking suback responses. */
            xFailedSubscribeToTopic = pdFALSE;

        }
    } while( ( xFailedSubscribeToTopic == pdTRUE ) && --retry );

    return xStatus;
}
/*-----------------------------------------------------------*/

static BaseType_t prvMQTTPublishToTopic( char *topic, uint8_t *data, uint16_t len, int QoS)
{
    MQTTStatus_t xResult;
    MQTTPublishInfo_t xMQTTPublishInfo= {0};
    BaseType_t xStatus = pdPASS;

    /* This demo uses QoS1. */
    xMQTTPublishInfo.qos = QoS;
    xMQTTPublishInfo.retain = false;
    xMQTTPublishInfo.pTopicName = topic;
    xMQTTPublishInfo.topicNameLength = ( uint16_t ) strlen( topic );
    xMQTTPublishInfo.pPayload = data;
    xMQTTPublishInfo.payloadLength = len;
    info("Try to publish to topic: %s\n", topic);
    usPublishPacketIdentifier=MQTT_GetPacketId( &mqtt.MQTTContext );
    xResult = MQTT_Publish( &mqtt.MQTTContext, &xMQTTPublishInfo, usPublishPacketIdentifier );

    if( xResult != MQTTSuccess )
    {
        xStatus = pdFAIL;
        error ( "Failed to send PUBLISH message to broker: Topic=%s, Error=%s\n", topic, MQTT_Status_strerror( xResult ) );
    }

    return xStatus;
}

/*-----------------------------------------------------------*/

static void prvMQTTProcessResponse( MQTTPacketInfo_t * pxIncomingPacket,
                                    uint16_t usPacketId )
{
    switch( pxIncomingPacket->type )
    {
        case MQTT_PACKET_TYPE_PUBACK:
            info ( "PUBACK received for packet Id %u.\n", usPacketId ) ;
            /* Make sure ACK packet identifier matches with Request packet identifier. */
//            configASSERT( usPublishPacketIdentifier == usPacketId );
            break;

        case MQTT_PACKET_TYPE_SUBACK:

            /* Update the packet type received to SUBACK. */
            usPacketTypeReceived = MQTT_PACKET_TYPE_SUBACK;
            prvUpdateSubAckStatus( pxIncomingPacket );

 
            if( mqtt.topic.xSubAckStatus != MQTTSubAckFailure )
            {
                info ( "Subscribed to the topic %s with maximum QoS %u.\n",
                            mqtt.topic.pcTopicFilter,
                            mqtt.topic.xSubAckStatus ) ;
            }

            /* Make sure ACK packet identifier matches with Request packet identifier. */
            configASSERT( usSubscribePacketIdentifier == usPacketId );
            break;

        case MQTT_PACKET_TYPE_UNSUBACK:
            info ( "Unsubscribed from the topic \n" ) ;

            /* Update the packet type received to UNSUBACK. */
            usPacketTypeReceived = MQTT_PACKET_TYPE_UNSUBACK;

            /* Make sure ACK packet identifier matches with Request packet identifier. */
            configASSERT( usUnsubscribePacketIdentifier == usPacketId );
            break;

        case MQTT_PACKET_TYPE_PINGRESP:
            info ( "Ping Response successfully received." ) ;

            break;

        /* Any other packet type is invalid. */
        default:
            warning( "prvMQTTProcessResponse() called with unknown packet type:(%02X).\n",
                       pxIncomingPacket->type );
    }
}


/*-----------------------------------------------------------*/

static void prvEventCallback( MQTTContext_t * pxMQTTContext,
                              MQTTPacketInfo_t * pxPacketInfo,
                              MQTTDeserializedInfo_t * pxDeserializedInfo )
{
    /* The MQTT context is not used for this demo. */
    ( void ) pxMQTTContext;

    if( ( pxPacketInfo->type & 0xF0U ) == MQTT_PACKET_TYPE_PUBLISH )
    {
        prvMQTTProcessIncomingPublish( pxDeserializedInfo->pPublishInfo );
    }
    else
    {
        prvMQTTProcessResponse( pxPacketInfo, pxDeserializedInfo->packetIdentifier );
    }
}

/*-----------------------------------------------------------*/

static uint32_t prvGetTimeMs( void )
{
    TickType_t xTickCount = 0;
    uint32_t ulTimeMs = 0UL;

    /* Get the current tick count. */
    xTickCount = xTaskGetTickCount();

    /* Convert the ticks to milliseconds. */
    ulTimeMs = ( uint32_t ) xTickCount * MILLISECONDS_PER_TICK;

    /* Reduce ulGlobalEntryTimeMs from obtained time so as to always return the
     * elapsed time in the application. */
    ulTimeMs = ( uint32_t ) ( ulTimeMs - ulGlobalEntryTimeMs );

    return ulTimeMs;
}

/*-----------------------------------------------------------*/

static MQTTStatus_t prvWaitForPacket( MQTTContext_t * pxMQTTContext,
                                      uint16_t usPacketType )
{
    uint8_t ucCount = 0U;
    MQTTStatus_t xMQTTStatus = MQTTSuccess;

    /* Reset the packet type received. */
    usPacketTypeReceived = 0U;
    uint32_t tick=millis();
    while( ( usPacketTypeReceived != usPacketType ) &&
           ( xMQTTStatus == MQTTSuccess )  && ((millis()-tick) <MQTT_PROCESS_LOOP_TIMEOUT_MS))
    {
        /* Event callback will set #usPacketTypeReceived when receiving appropriate packet. This
         * will wait for at most . */
        xMQTTStatus = MQTT_ProcessLoop( pxMQTTContext, MQTT_PROCESS_LOOP_TIMEOUT_MS );
        delay(5);
    }

    if( ( xMQTTStatus != MQTTSuccess ) || ( usPacketTypeReceived != usPacketType ) )
    {
        error( "MQTT_ProcessLoop failed to receive packet: Packet type=%02X, LoopDuration=%u, Status=%s\n",
                    usPacketType,
                    ( MQTT_PROCESS_LOOP_TIMEOUT_MS * ucCount ),
                    MQTT_Status_strerror( xMQTTStatus ) ) ;
    }

    return xMQTTStatus;
}


/*-----------------------------------------------------------*/
#ifdef JIGTEST
static void jigtest_mqtt_start_test()
{
	test_tick=millis();
	test_count++;
	prvMQTTPublishToTopic(test_topic, (uint8_t *)test_message, strlen(test_message), MQTTQoS0);
}

static void jigtest_mqtt_test_result()
{
	info("Mqtt test done result: %d/%d\n", test_success, test_num);
	mqtt_test_done=true;
	if(test_success > test_num/2)
	{
		jigtest_direct_report(UART_UI_RES_MQTT_TEST, 1);
	}
	else
	{
		jigtest_direct_report(UART_UI_RES_MQTT_TEST, 0);
	}
}

static void jigtest_mqtt_event_handle()
{
	if(test_count<test_num)
	{
		jigtest_mqtt_start_test();
	}
	else
	{
		jigtest_mqtt_test_result();
	}
}
#endif

static void prvMQTTProcessIncomingPublish( MQTTPublishInfo_t * pxPublishInfo )
{
    configASSERT( pxPublishInfo != NULL );

	debug ( "Incoming message Topic: %.*s\n",
			   pxPublishInfo->topicNameLength,
			   pxPublishInfo->pTopicName);

	const char *result=pxPublishInfo->pTopicName;
	if(__check_cmd(cmd_topic)){
		debug("Device command topic\n");
		uint8_t *frame=malloc(pxPublishInfo->payloadLength);
		if(frame==NULL){
			error("Malloc\n");
			return;
		}
		memcpy(frame, pxPublishInfo->pPayload, pxPublishInfo->payloadLength);
		mail_t mail={.type=MAIL_MQTT_V2_SERVER_DATA, .data=frame, .len=pxPublishInfo->payloadLength};
		if(osMessageQueuePut(mainMailHandle, &mail, 0, 10)!=osOK){
			error("Mail put error\n");
			free(frame);
		}
	}
#ifdef JIGTEST
	else if(__check_cmd(test_topic))
	{
		if(pxPublishInfo->payloadLength==strlen(test_message) &&\
				memcmp(pxPublishInfo->pPayload, test_message, strlen(test_message))==0)
		{
			test_success++;
			info("test success num: %d\n", test_success);
		}
		jigtest_mqtt_event_handle();
	}
#endif
}

//bool mqtt_regist_message_callback(mqtt_msg_callback_t callback){
//    mqtt.message_cb=callback;
//    return true;
//}
/*-----------------------------------------------------------*/
typedef struct{
	char *topic;
    uint8_t *data;
    __IO size_t len;
    int QoS;
    bool result;
}mqtt_pulish_request_t;


/*-----------------------------------------------------------*/
bool mqtt_init( ){
	lara_r2_socket_init(&mqtt.socket, SOCKET_TCP);
	mqtt.NetworkContext.socket=&mqtt.socket;
	configASSERT(mqtt.NetworkContext.socket!=NULL);
	sprintf(client_id, "%s%s", CLIENT_IDENTIFIER_PREFIX, lteImei);
	sprintf(status_topic, "%s%s", STATUS_TOPIC_PREFIX, lteImei);
	sprintf(cmd_topic, "%s%s", CMD_TOPIC_PREFIX, lteImei);
	sprintf(ack_topic, "%s%s", ACK_TOPIC_PREFIX, lteImei);
#ifdef JIGTEST
	sprintf(test_topic, "%s%s", TEST_TOPIC_PREFIX, lteImei);
#endif
}

bool mqtt_start(){
	info("MQTT start connect\n");
	if( !lara_r2_socket_connect(mqtt.NetworkContext.socket, factory_config->broker.endpoint, factory_config->broker.port, factory_config->broker.secure ))
	{
		error("Failed to create socket\n");
		return false;
	}
	info ( "Creating an MQTT connection to %s.\n", factory_config->broker.endpoint ) ;
	if(pdPASS != prvCreateMQTTConnectionWithBroker( &mqtt.MQTTContext, &mqtt.NetworkContext )){
		goto __exit;
	}
	if(pdPASS != prvMQTTSubscribe( cmd_topic )){
		goto __exit;
	}
#ifdef JIGTEST
	if(pdPASS!=prvMQTTSubscribe(test_topic))
	{
		goto __exit;
	}
	mqtt_test_done=false;
	mqtt_test_result=false;
	test_count=0;
	test_success=0;
	jigtest_mqtt_start_test();
#endif
	return true;
	__exit:
	lara_r2_socket_close(&mqtt.socket);
	return false;
}

void mqtt_task_process( )
{
	if(mqtt.publish_failed>5){
		mqtt.publish_failed=0;
		info("Request to restart network cause public fail too many time\n");
		mqtt_ready=false;
		mail_t mail={.type=MAIL_LTE_NETWORK_RESTART, .data=NULL, .len=0};
		osMessageQueuePut(lteMailHandle, &mail, 0, 10);
	}
	MQTT_ProcessLoop( &mqtt.MQTTContext , 0);
}

static void mqttMailProcess(){
	static mail_t mail;
	if(osOK!=osMessageQueueGet(mqttMailHandle, &mail, NULL, 0)){
		return;
	}
	debug("Handle mqtt mail\n");
	if(!mqtt_ready){
		debug("Mqtt not ready, remove packet\n");
		goto __exit;
	}
	if(mail.type==MAIL_MQTT_V1_PUBLISH_FRAME){
		mqtt_publish("/data/status", mail.data, mail.len, MQTTQoS0);
	}
	else if(mail.type==MAIL_MQTT_V2_PUBLISH){
		mqtt_publish(status_topic, mail.data, mail.len, MQTTQoS0);
	}
	else if(mail.type==MAIL_MQTT_V2_ACK){
		mqtt_publish(ack_topic, mail.data, mail.len, MQTTQoS0);
	}
	__exit:
	if(mail.data!=NULL){
		free(mail.data);
		mail.data=NULL;
	}
}

#ifdef JIGTEST
void jigtest_mqtt_polling()
{
	if(millis()-test_tick>5000 && !mqtt_test_done)
	{
		debug("mqtt test timeout\n");
		jigtest_mqtt_event_handle();
	}
}
#endif

void app_mqtt()
{
#ifdef MQTT_ENABLE
	while(!network_is_ready()){
		delay(5);
	}
	debug("Mqtt task start\n");
	mqtt_init();
	uint8_t count=0;
	__mqtt_start:
	while(true){
		count=0;
		while(!network_is_ready()){
			delay(5);
		}
		if(mqtt_start()){
			mqtt_ready=true;
			break;
		}
		count++;
		if(count==3){
			mail_t mail={.type=MAIL_LTE_NETWORK_RESTART, .data=NULL, .len=0};
			if(osMessageQueuePut(lteMailHandle, &mail, 0, 10)!=osOK){
				error("mail put failed\n");
			}
			count=0;
		}
		delay(10000);
	}
	while(true){

		if(mqtt.NetworkContext.socket->state!=SOCKET_CONNECTED){
			debug("Socket disconnected\n");
			mqtt_ready=false;
			goto __mqtt_start;
		}
		else if(!network_is_ready()){
			debug("network error\n");
			mqtt_ready=false;
			goto __mqtt_start;
		}
#ifdef JIGTEST
		jigtest_mqtt_polling();
#endif
		mqtt_task_process();
		mqttMailProcess();
		delay(5);
	}
#endif
}

void app_mqtt_console_handle(char *result)
{
	if(__check_cmd("set ready "))
	{
		int x;
		if(sscanf(__param_pos("set ready "), "%d", &x)==1)
		{
			debug("Set mqtt ready to: %d\n", x);
			mqtt_ready=x;
		}
		else debug("Params error\n");
	}
	else debug("Unknown cmd\n");
}
