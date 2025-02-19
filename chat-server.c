
#include "http-server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define USERNAME_SIZE 16      
#define MESSAGE_SIZE 256       
#define REACTION_SIZE 16      
#define TIMESTAMP_SIZE 20     
#define MAX_REACTIONS 100      
#define CHAT_BUFFER_SIZE 8200  
#define MAX_CHATS 100000         

char const HTTP_404_NOT_FOUND[] = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n";
char const HTTP_200_OK[] = "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\n\r\n";
char const HTTP_400_BAD_REQUEST[] = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\n";
char const HTTP_500_INTERNAL_SERVER_ERROR[] = "HTTP/1.1 500 Internal Server Error\r\nContent-Type: text/plain\r\n\r\n";


void parse_query_parameters(char *query, char *user, char *message, char *id);
void handle_chats(int client_sock);
void handle_post(int client_sock, char *path);
void handle_react(int client_sock, char *path);
void handle_reset(int client_sock);
void handle_edit(int client_sock, char *path);

typedef struct {
    char user[USERNAME_SIZE];
    char message[REACTION_SIZE];
} Reaction;

typedef struct Chat{
    uint32_t id;
    char user[USERNAME_SIZE];
    char message[MESSAGE_SIZE];
    char timestamp[TIMESTAMP_SIZE];
    uint32_t num_reactions;
    Reaction reactions[MAX_REACTIONS];
} Chat;

Chat chat_storage[MAX_CHATS];  
int chat_count = 0; 

void handle_404(int client_sock, char *path)  {
    printf("SERVER LOG: Got request for unrecognized path \"%s\"\n", path);

    char response_buff[CHAT_BUFFER_SIZE];
    snprintf(response_buff, CHAT_BUFFER_SIZE, "Error 404:\r\nUnrecognized path \"%s\"\r\n", path);
 

    write(client_sock, HTTP_404_NOT_FOUND, strlen(HTTP_404_NOT_FOUND));
    write(client_sock, response_buff, strlen(response_buff));
}


void url_decode(char *dest, const char *src) {
    char *d = dest;
    const char *s = src;
    while (*s) {
        if (*s == '%') {
            if (isxdigit(*(s + 1)) && isxdigit(*(s + 2))) {
                char hex[3] = { *(s + 1), *(s + 2), '\0' };
                *d++ = (char)strtol(hex, NULL, 16);
                s += 3;
            } else {
                *d++ = *s++;
            }
        } else if (*s == '+') {
            *d++ = ' ';
            s++;
        } else {
            *d++ = *s++;
        }
    }
    *d = '\0';
}

void handle_chats(int client_sock) {
    char response_buff[CHAT_BUFFER_SIZE];
    int offset = 0;

    write(client_sock, HTTP_200_OK, strlen(HTTP_200_OK));

    for (int i = 0; i < chat_count; i++) {
        Chat *chat = &chat_storage[i];

        offset = snprintf(response_buff, CHAT_BUFFER_SIZE,
                "[#%d %s]   %s: %s\n",
                chat->id, chat->timestamp, chat->user, chat->message);
        write(client_sock, response_buff, offset);

        for (uint32_t j = 0; j < chat->num_reactions; j++) {
            Reaction *reaction = &chat->reactions[j];
            offset = snprintf(response_buff, CHAT_BUFFER_SIZE,
                              "                    (%s)  %s\n",
                              reaction->user, reaction->message);
            write(client_sock, response_buff, offset);
        }
    }
}

void handle_response(char *request, int client_sock) {
    char path[256];
    printf("\nSERVER LOG: Got request: \"%s\"\n", request);
    if (sscanf(request, "GET %255s", path) != 1) {
        printf("Invalid request line\n");
        handle_404(client_sock, path);
        return;
    }

    if (strcmp(path, "/chats") == 0) {
        handle_chats(client_sock);
    } else if (strncmp(path, "/post", 5) == 0) {
        handle_post(client_sock, path);
    } else if (strncmp(path, "/react", 6) == 0) {
        handle_react(client_sock, path);
    } else if (strncmp(path, "/edit", 5) == 0) { 
        handle_edit(client_sock, path);
    } else if (strcmp(path, "/reset") == 0) {
        handle_reset(client_sock);
    } else {
        handle_404(client_sock, path);
    }
}


void parse_query_parameters(char *query, char *user, char *message, char *id) {
    char *token = strtok(query, "&");
    while (token != NULL) {
        if (strncmp(token, "user=", 5) == 0) {
            url_decode(user, token + 5);
        } else if (strncmp(token, "message=", 8) == 0) {
            url_decode(message, token + 8);
        } else if (strncmp(token, "id=", 3) == 0) {
            strncpy(id, token + 3, 10);
            id[10] = '\0';
        }
        token = strtok(NULL, "&");
    }
}

void handle_post(int client_sock, char *path) {
    char user_encoded[USERNAME_SIZE * 3] = "";  
    char message_encoded[MESSAGE_SIZE * 3] = "";
    char user[USERNAME_SIZE] = "";
    char message[MESSAGE_SIZE] = "";

    char *query = strchr(path, '?');
    if (query != NULL) {
        query++;  
        parse_query_parameters(query, user_encoded, message_encoded, NULL);
    } else {
        write(client_sock, HTTP_400_BAD_REQUEST, strlen(HTTP_400_BAD_REQUEST));
        write(client_sock, "Missing query parameters.\n", 26);
        return;
    }

    url_decode(user, user_encoded);
    url_decode(message, message_encoded);

    if (strlen(user) == 0 || strlen(message) == 0) {
        write(client_sock, HTTP_400_BAD_REQUEST, strlen(HTTP_400_BAD_REQUEST));
        write(client_sock, "Missing 'user' or 'message' parameter.\n", 39);
        return;
    }

    if (strlen(user) >= USERNAME_SIZE || strlen(message) >= MESSAGE_SIZE) {
        write(client_sock, HTTP_400_BAD_REQUEST, strlen(HTTP_400_BAD_REQUEST));
        write(client_sock, "Parameter 'user' or 'message' too long.\n", 40);
        return;
    }

    if (chat_count >= MAX_CHATS) {
        write(client_sock, HTTP_500_INTERNAL_SERVER_ERROR, strlen(HTTP_500_INTERNAL_SERVER_ERROR));
        write(client_sock, "Chat limit exceeded.\n", 21);
        return;
    }

        Chat new_chat;
    new_chat.id = chat_count + 1;
    strncpy(new_chat.user, user, USERNAME_SIZE - 1);
    new_chat.user[USERNAME_SIZE - 1] = '\0';
    strncpy(new_chat.message, message, MESSAGE_SIZE - 1);
    new_chat.message[MESSAGE_SIZE - 1] = '\0';

    // Use strftime to format the timestamp
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(new_chat.timestamp, TIMESTAMP_SIZE, "%Y-%m-%d %H:%M:%S", t);

    new_chat.num_reactions = 0;
    chat_storage[chat_count++] = new_chat;

    handle_chats(client_sock);

}

void handle_edit(int client_sock, char *path) {
    char id_str[11] = ""; 
    char message_encoded[MESSAGE_SIZE * 3] = ""; 
    char message[MESSAGE_SIZE] = ""; 
    uint32_t chat_id;

  
    char *query = strchr(path, '?');
    if (query != NULL) {
        query++;
        char query_copy[256];
        strncpy(query_copy, query, sizeof(query_copy) - 1);
        query_copy[sizeof(query_copy) - 1] = '\0';
        parse_query_parameters(query_copy, NULL, message_encoded, id_str);
    } else {
        write(client_sock, HTTP_400_BAD_REQUEST, strlen(HTTP_400_BAD_REQUEST));
        write(client_sock, "Missing query parameters.\n", 26);
        return;
    }

    
    url_decode(message, message_encoded);

    
    if (strlen(message) == 0 || strlen(id_str) == 0) {
        write(client_sock, HTTP_400_BAD_REQUEST, strlen(HTTP_400_BAD_REQUEST));
        write(client_sock, "Missing 'message' or 'id' parameter.\n", 39);
        return;
    }

    if (strlen(message) >= MESSAGE_SIZE) {
        write(client_sock, HTTP_400_BAD_REQUEST, strlen(HTTP_400_BAD_REQUEST));
        write(client_sock, "Message parameter too long.\n", 29);
        return;
    }

    chat_id = atoi(id_str);
    if (chat_id == 0 || chat_id > chat_count) {
        write(client_sock, HTTP_400_BAD_REQUEST, strlen(HTTP_400_BAD_REQUEST));
        write(client_sock, "Invalid chat ID.\n", 17);
        return;
    }

    
    Chat *chat = &chat_storage[chat_id - 1];
    strncpy(chat->message, message, MESSAGE_SIZE - 1);
    chat->message[MESSAGE_SIZE - 1] = '\0';

   
    handle_chats(client_sock);
}



void handle_react(int client_sock, char *path) {
    char user_encoded[USERNAME_SIZE * 3] = "";
    char message_encoded[REACTION_SIZE * 3] = "";
    char id_str[11] = "";
    char user[USERNAME_SIZE] = "";
    char message[REACTION_SIZE] = "";
    uint32_t chat_id;

    char *query = strchr(path, '?');
    if (query != NULL) {
        query++;
        char query_copy[256];
        strncpy(query_copy, query, sizeof(query_copy) - 1);
        query_copy[sizeof(query_copy) - 1] = '\0';
        parse_query_parameters(query_copy, user_encoded, message_encoded, id_str);
    } else {
        write(client_sock, HTTP_400_BAD_REQUEST, strlen(HTTP_400_BAD_REQUEST));
        write(client_sock, "Missing query parameters.\n", 26);
        return;
    }

    url_decode(user, user_encoded);
    url_decode(message, message_encoded);

    if (strlen(user) == 0 || strlen(message) == 0 || strlen(id_str) == 0) {
        write(client_sock, HTTP_400_BAD_REQUEST, strlen(HTTP_400_BAD_REQUEST));
        write(client_sock, "Missing 'user', 'message', or 'id' parameter.\n", 46);
        return;
    }

    if (strlen(user) >= USERNAME_SIZE) {
        write(client_sock, HTTP_400_BAD_REQUEST, strlen(HTTP_400_BAD_REQUEST));
        write(client_sock, "Username too long.\n", 19);
        return;
    }

    if (strlen(message) >= REACTION_SIZE) {
        write(client_sock, HTTP_400_BAD_REQUEST, strlen(HTTP_400_BAD_REQUEST));
        write(client_sock, "Reaction message too long.\n", 27);
        return;
    }

    chat_id = atoi(id_str);
    if (chat_id == 0 || chat_id > chat_count) {
        write(client_sock, HTTP_400_BAD_REQUEST, strlen(HTTP_400_BAD_REQUEST));
        write(client_sock, "Invalid chat ID.\n", 17);
        return;
    }

    Chat *chat = &chat_storage[chat_id - 1];
    if (chat->num_reactions < MAX_REACTIONS) {
        Reaction new_reaction;
        strncpy(new_reaction.user, user, USERNAME_SIZE - 1);
        new_reaction.user[USERNAME_SIZE - 1] = '\0';
        strncpy(new_reaction.message, message, REACTION_SIZE - 1);
        new_reaction.message[REACTION_SIZE - 1] = '\0';
        chat->reactions[chat->num_reactions++] = new_reaction;

        handle_chats(client_sock);  
    } else {
        write(client_sock, HTTP_400_BAD_REQUEST, strlen(HTTP_400_BAD_REQUEST));
        write(client_sock, "Maximum number of reactions reached.\n", 38);
    }
}

void handle_reset(int client_sock) {
    chat_count = 0;
    write(client_sock, HTTP_200_OK, strlen(HTTP_200_OK));
   
}
//-----------------------------------INT MAIN-------------------------------------------------------
int main(int argc, char *argv[]) {
    int port = 0;
    if(argc >= 2) { 
        port = atoi(argv[1]);
    }

    start_server(&handle_response, port);
}