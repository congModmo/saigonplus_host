/*
    Utils

    07/01/2021
    HieuNT
*/
#include "utils.h"

int str_token(char *s, char *set, char **token, int maxSz)
{
	char *tok;
	int count = 0;
  tok = strtok (s, set);
  while (tok != NULL) {
    if (count < maxSz){
    	*token++ = tok;	
    	count++;
    }
    tok = strtok (NULL, set);
  }

  return count;
}
