#ifndef DHT22_H
#define DHT22_H

typedef struct {
    float humidity;
    float temperature;
} dht_data;

bool read_from_dht(dht_data *result);


#endif