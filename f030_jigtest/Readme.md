# Rules

Vì chưa biết sử dụng chip nào trong 3 loại chip stm32f030, stm32f303, gd32f303, nên port lại project để hardware independent.

Quy ước, sử dụng thư viện chuẩn HAL, chip gd32 không xài HAL thì tạo lớp layer để port HAL qua.

Mỗi lần sửa cấu hình gì thì sửa cube MX rồi generate ra, không sửa trực tiếp code.
![Structure](./Images/structure.png)