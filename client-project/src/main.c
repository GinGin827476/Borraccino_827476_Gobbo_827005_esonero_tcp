#include "protocol.h"

void errorhandler(char *msg) {
    printf("%s", msg);
}

void clearwinsock() {
#if defined WIN32
    WSACleanup();
#endif
}

/* Rende la città in formato "Bari", "Reggio Calabria" */
void normalize_city(char *s) {
    int neww = 1;
    int i;
    for (i = 0; s[i]; i++) {
        if (s[i] == ' ') neww = 1;
        else {
            if (neww) s[i] = (char)toupper(s[i]);
            else s[i] = (char)tolower(s[i]);
            neww = 0;
        }
    }
}

/* parsing di -r "t roma" */
int parse_req(const char *input, weather_request_t *req) {
    if (!input || strlen(input) < 1) return 0;

    req->type = input[0];

    if (strlen(input) >= 2) {
        strncpy(req->city, input + 1, 63);
        req->city[63] = '\0';
        while (*req->city == ' ') {
            int len = strlen(req->city);
            memmove(req->city, req->city + 1, len);
        }
    } else {
        req->city[0] = '\0';
    }
    return 1;
}

int main(int argc, char *argv[]) {

#if defined WIN32
    WSADATA wsa_data;
    WORD version_requested = MAKEWORD(2,2);
    int r = WSAStartup(version_requested, &wsa_data);
    if (r != NO_ERROR) {
        printf("Error at WSAStartup()\n");
        return 1;
    }
#endif

    const char *server_ip = DEFAULT_IP;
    int port = DEFAULT_PORT;
    const char *req_str = NULL;

    int i;
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-s") == 0 && i+1 < argc)
            server_ip = argv[++i];
        else if (strcmp(argv[i], "-p") == 0 && i+1 < argc)
            port = atoi(argv[++i]);
        else if (strcmp(argv[i], "-r") == 0 && i+1 < argc)
            req_str = argv[++i];
        else {
            printf("Uso: %s [-s server] [-p port] -r \"type city\"\n", argv[0]);
            return 1;
        }
    }

    if (!req_str) {
        printf("Uso: %s [-s server] [-p port] -r \"type city\"\n", argv[0]);
        return 1;
    }

    weather_request_t req;
    if (!parse_req(req_str, &req)) {
        printf("Uso non valido della richiesta.\n");
        return 1;
    }

    int c_sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (c_sock < 0) {
        errorhandler("socket() failed.\n");
        clearwinsock();
        return 1;
    }

    struct sockaddr_in sad;
    memset(&sad, 0, sizeof(sad));
    sad.sin_family = AF_INET;
    sad.sin_addr.s_addr = inet_addr(server_ip);
    sad.sin_port = htons(port);

    if (connect(c_sock, (struct sockaddr*)&sad, sizeof(sad)) < 0) {
        errorhandler("connect() failed.\n");
        closesocket(c_sock);
        clearwinsock();
        return 1;
    }

    send(c_sock, &req, sizeof(req), 0);

    weather_response_t res;
    int rcvd = recv(c_sock, &res, sizeof(res), 0);
    if (rcvd != sizeof(res)) {
        printf("Errore nella risposta.\n");
        closesocket(c_sock);
        clearwinsock();
        return 1;
    }

    printf("Ricevuto risultato dal server ip %s. ", server_ip);

    if (res.status == STATUS_OK) {
        char city_fmt[64];
        strncpy(city_fmt, req.city, 63);
        city_fmt[63] = '\0';
        normalize_city(city_fmt);

        if (res.type == TYPE_TEMPERATURE)
            printf("%s: Temperatura = %.1f°C", city_fmt, res.value);
        else if (res.type == TYPE_HUMIDITY)
            printf("%s: Umidità = %.1f%%", city_fmt, res.value);
        else if (res.type == TYPE_WIND)
            printf("%s: Vento = %.1f km/h", city_fmt, res.value);
        else if (res.type == TYPE_PRESSURE)
            printf("%s: Pressione = %.1f hPa", city_fmt, res.value);
        else
            printf("Richiesta non valida");

    } else if (res.status == STATUS_CITY_NOT_FOUND) {
        printf("Città non disponibile");
    } else {
        printf("Richiesta non valida");
    }

    printf("\n");

    closesocket(c_sock);
    clearwinsock();
    return 0;
}


