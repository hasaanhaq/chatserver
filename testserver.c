


#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "http-server.h"

#define MAX_CHATS 100000
#define MAX_USERNAME_LENGTH 15
#define MAX_MESSAGE_LENGTH 255
#define ASSERTIONS_ENABLED 1

typedef struct {
    char username[MAX_USERNAME_LENGTH + 1];
    char message[MAX_MESSAGE_LENGTH + 1];
    int id;

} Chat;

Chat chat_log[MAX_CHATS];
int chat_count = 0;
static int chat_id = 1;

uint8_t add_chat(char* username, char* message){

    if (username == NULL || message == NULL){
        printf("ERROR: MISSING USERNAME AND OR MESSAGE\n"); //check if there is a username and message
        return 1;  // Mapped to ERROR_MISSING_PARAM
    }

    if(strlen(username) > MAX_USERNAME_LENGTH || strlen(message) > MAX_MESSAGE_LENGTH){
        printf("ERROR: USERNAME OR MESSAGE TOO LONG\n"); //check if the username or message is too long
        return 2;  // Mapped to ERROR_TOO_LONG
    }

    if(chat_count >= MAX_CHATS){
        printf("ERROR: CHAT LOG FULL\n"); //check if the chat log is full
        return 3;  // Mapped to ERROR_CHAT_LOG_FULL
    }

    Chat new_chat;
    new_chat.id = chat_id++;
    strncpy(new_chat.username, username, MAX_USERNAME_LENGTH);
    new_chat.username[MAX_USERNAME_LENGTH] = '\0'; //making sure it null terminates
    strncpy(new_chat.message, message, MAX_MESSAGE_LENGTH);
    new_chat.message[MAX_MESSAGE_LENGTH] = '\0'; //making sure it null terminates

    chat_log[chat_count++] = new_chat;

    return 0;
}

void test_add_chat() {
#if ASSERTIONS_ENABLED == 1
    // Test 1: Valid username and message
    assert(add_chat("user1", "Hello, world!") == 0);
    assert(chat_log[0].id == 1);
    assert(strcmp(chat_log[0].username, "user1") == 0);
    assert(strcmp(chat_log[0].message, "Hello, world!") == 0);

    // Test 2: Adding a new valid username and message
    assert(add_chat("user2", "New message!") == 0);
    assert(chat_log[1].id == 2);
    assert(strcmp(chat_log[1].username, "user2") == 0);
    assert(strcmp(chat_log[1].message, "New message!") == 0);

    // Test 3: Missing username
    assert(add_chat(NULL, "Hello!") == 1);

    // Test 4: Missing message
    assert(add_chat("user3", NULL) == 1);

    // Test 5: Username too long
    assert(add_chat("thisusernameiswaytoolong", "Hello!") == 2);

    // Test 6: Message too long
    char long_message[MAX_MESSAGE_LENGTH + 10];
    memset(long_message, 'a', MAX_MESSAGE_LENGTH + 9);  // Exceeds limit
    long_message[MAX_MESSAGE_LENGTH + 9] = '\0';
    assert(add_chat("user4", long_message) == 2);

    // Test 7: Maximum chat count exceeded
    chat_count = MAX_CHATS;  // Simulate full chat log
    assert(add_chat("user5", "This won't be added") == 3);

    // Reset for future tests
    chat_count = 0;
    chat_id = 1;

    printf("All tests passed!\n");
    printf("%s\n", chat_log[1].username);
#endif
}

int main() {
    test_add_chat();
    return 0;
}
