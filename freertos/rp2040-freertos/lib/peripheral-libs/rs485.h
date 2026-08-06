#ifndef RS485_H
#define RS485_H

typedef struct {
    float tem;
    float hem;
    float ph;
} rs485_data;

void readHumiturePH(rs485_data *result);
uint8_t readN(uint8_t *buf, size_t len);
unsigned int CRC16_2(unsigned char *buf, int len);
void rs485_init(void);
void rs485_send_command(const uint8_t *buf, size_t len);
#endif