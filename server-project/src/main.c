#include "protocol.h"
#include <time.h>

#ifndef WIN32
#include <unistd.h>
#endif

#define QLEN 6

void errorhandler(char *msg) {
    printf("%s", msg);
}

void clearwinsock() {
#if defined WIN32
    WSACleanup();
#endif
}

// RANDOM FLOAT [MIN, MAX]
float frand(float min, float max) {
    float r = (float)rand() / (float)RAND_MAX;
    return min + r * (max - min);
}

float get_temperature(void) {
    return frand(-10.0f, 40.0f);
}

float get_humidity(void) {
    return frand(20.0f, 100.0f);
}

float get_wind(void) {
    return frand(0.0f, 100.0f);
}

float get_pressure(void) {
    return frand(950.0f, 1050.0f);
}

int equals_ic(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return (*a == '\0' && *b == '\0');
}

int is_supported_city(const char *c) {

    const char *list[] = {
        "Bari", "Roma", "Milano", "Napoli", "Torino",
        "Palermo", "Genova", "Bologna", "Firenze", "Venezia"
    };

    int i;
    for (i = 0; i < 10; i++) {
        if (equals_ic(c, list[i])) return 1;
    }
    return 0;
}

int is_type_valid(char t) {
    return (t == TYPE_TEMPERATURE ||
            t == TYPE_HUMIDITY ||
            t == TYPE_WIND ||
            t == TYPE_PRESSURE);
}

int main(int argc, char *argv[]) {

#if defined WIN32
    WSADATA wsa_data;
    WORD version_requested = MAKEWORD(2, 2);
    int res = WSAStartup(version_requested, &wsa_data);
    if (res != NO_ERROR) {
        printf("Error at WSAStartup()\n");
        return 1;
    }
#endif

    srand((unsigned)time(NULL));

    int port = DEFAULT_PORT;
    int i;

    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            port = atoi(argv[++i]);
        } else {
            printf("Uso: %s [-p port]\n", argv[0]);
            clearwinsock();
            return 1;
        }
    }

    // CREAZIONE DELLA SOCKET
    int serv_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serv_sock < 0) {
        errorhandler("socket() failed.\n");
        clearwinsock();
        return 1;
    }

    // INDIRIZZO
    struct sockaddr_in sad;
    memset(&sad, 0, sizeof(sad));
    sad.sin_family = AF_INET;
    sad.sin_addr.s_addr = htonl(INADDR_ANY);
    sad.sin_port = htons(port);

    if (bind(serv_sock, (struct sockaddr*) &sad, sizeof(sad)) < 0) {
        errorhandler("bind() failed.\n");
        closesocket(serv_sock);
        clearwinsock();
        return 1;
    }

    if (listen(serv_sock, QLEN) < 0) {
        errorhandler("listen() failed.\n");
        closesocket(serv_sock);
        clearwinsock();
        return 1;
    }

    printf("Server in ascolto sulla porta %d...\n", port);

    while (1) {
        struct sockaddr_in cad;
        socklen_t cad_len = sizeof(cad);

        int client_sock = accept(serv_sock, (struct sockaddr*)&cad, &cad_len);
        if (client_sock < 0) {
            errorhandler("accept() failed.\n");
            continue;
        }

        char *client_ip = inet_ntoa(cad.sin_addr);

        weather_request_t req;
        int rcvd = recv(client_sock, &req, sizeof(req), 0);
        if (rcvd != sizeof(req)) {
            closesocket(client_sock);
            continue;
        }

        req.city[63] = '\0';

        printf("Richiesta '%c %s' dal client ip %s\n",
               req.type, req.city, client_ip);

        weather_response_t res_msg;

        if (!is_type_valid(req.type)) {
            res_msg.status = STATUS_BAD_REQUEST;
            res_msg.type = '\0';
            res_msg.value = 0.0f;
        } else if (!is_supported_city(req.city)) {
            res_msg.status = STATUS_CITY_NOT_FOUND;
            res_msg.type = '\0';
            res_msg.value = 0.0f;
        } else {
            res_msg.status = STATUS_OK;
            res_msg.type = req.type;

            if (req.type == TYPE_TEMPERATURE)      res_msg.value = get_temperature();
            else if (req.type == TYPE_HUMIDITY)    res_msg.value = get_humidity();
            else if (req.type == TYPE_WIND)        res_msg.value = get_wind();
            else if (req.type == TYPE_PRESSURE)    res_msg.value = get_pressure();
        }

        send(client_sock, &res_msg, sizeof(res_msg), 0);
        closesocket(client_sock);
    }

    closesocket(serv_sock);
    clearwinsock();
    return 0;
}

