#ifndef PROTOCOL_H
#define PROTOCOL_H

#if defined WIN32
#include <winsock.h>
#else
#define closesocket close
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


#define DEFAULT_PORT 56700
#define DEFAULT_IP "127.0.0.1"

#define STATUS_OK             0
#define STATUS_CITY_NOT_FOUND 1
#define STATUS_BAD_REQUEST    2


// TIPI VALIDI
#define TYPE_TEMPERATURE 't'
#define TYPE_HUMIDITY    'h'
#define TYPE_WIND        'w'
#define TYPE_PRESSURE     'p'

typedef struct {
    char type;
    char city[64];
} weather_request_t;

typedef struct {
    unsigned int status;
    char type;
    float value;
} weather_response_t;

//Generatori meteo
float get_temperature(void);
float get_humidity(void);
float get_wind(void);
float get_pressure(void);

#endif
