#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "udp.h"
#include "message.h"
#include "server_functions.h"

#define MAX_CLIENTS 100

typedef struct {
    uint32_t client_id;
    uint32_t last_sequence_number;
    message last_result;
} client_info;

typedef struct{
    message msg;
    struct packet_info packet;
    client_info* cli;
    struct socket server;
}thread_data;

client_info client_table[MAX_CLIENTS];

void *client_thread(void *arg) {
    printf("new thread\n");
    thread_data *data = (thread_data* ) arg;
    printf("client: address: %p, message_type: %d\n", data->cli, data->cli->last_result.message_type);
    // memcpy(data, arg, sizeof(thread_data));
    message incoming_message = data->msg;
    struct packet_info incoming_packet = data->packet;
    client_info *client = data->cli;
    struct socket server_socket = data->server;

    //uint32_t client_id = incoming_message.client_id;
    message response_msg = incoming_message;

    switch (incoming_message.message_type) {
        case GET_CALL:
            response_msg.value = get(incoming_message.key);
            response_msg.message_type = GET_RETURN;
            printf("value:%d\n",response_msg.value);
            break;
        case PUT_CALL:
            response_msg.value = put(incoming_message.key,incoming_message.value);
            response_msg.message_type = PUT_RETURN;
            printf("put call\n");
            break;
        case IDEL_CALL:
            idle(incoming_message.time);
            response_msg.message_type = IDEL_RETURN;
            break;
        default:
            printf("Unknown message type received.\n");
            break;
    }

    
    // set to incomeing_message.sequence_number in this case,
    // which is an error in the code. It should be set to the result of the operation
    // that was just performed by the server, based on the incoming message. The code should
    // look like this instead:
    client->last_result = response_msg;
    send_packet(server_socket, incoming_packet.sock, incoming_packet.slen, (char*)&response_msg , sizeof(response_msg));

    // Free the memory allocated for the incoming message
    printf("thread exit\n");
    // Exit the thread
    pthread_exit(NULL);
}


int main(int argc, char *argv[]) {

    if (argc != 2) {
        printf("Usage: %s <port>\n", argv[0]);
        return 1;
    }


    int port = atoi(argv[1]);
    struct socket server_socket = init_socket(port);

    memset(client_table, 0, sizeof(client_table));

    for (int i = 0; i < MAX_CLIENTS; i++) {
            client_table[i].client_id = -1;
    }

    while (1) {

        // initializes the UDP socket with the specified port and assigns the resulting
        // socket struct to the server_socket variable. 
        struct packet_info incoming_packet = receive_packet(server_socket);

        struct sockaddr client_socket = incoming_packet.sock;
        // deserialize the incoming packet's data from a buffer of bytes into a message struct
        // so that the server can access its fields, such as the message type, client ID, and arguments.

        message *incoming_message = malloc(sizeof(message));
        memcpy(incoming_message,incoming_packet.buf,sizeof(message));
        //message incoming_message = *incoming_message_ptr;

        // printf("client id:%d\n", incoming_message->client_id);
        /*TO DO create new thread to execute */

        // Find client in the client table
        client_info *client = NULL;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (client_table[i].client_id == incoming_message->client_id) {
                client = &client_table[i];
                // printf("clinet id:%d, i: %d\n",client->client_id, i);
                break;
            }
        }

        if (client == NULL) {
            // Client not found in client table, add them
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_table[i].client_id == -1) {
                    client_table[i].client_id = incoming_message->client_id;
                    client = &client_table[i];
                    // printf("clinet id:%d, i: %d\n",client->client_id, i);
                    break;
                }
            }
        }
        // printf("seq: %d, client: %d, type: %d, last_seq: %d\n", incoming_message->sequence_number, incoming_message->client_id, incoming_message->message_type, client->last_sequence_number);
        fflush(stdout);
        // printf("main: client: %p\n", (void*)client);
        if (incoming_message->sequence_number > client->last_sequence_number) {
            // updated to match the sequence_number of the incoming message, indicating that the
            // server has received and processed the current request from the client.
            client->last_sequence_number = incoming_message->sequence_number;
            // New request TODO set the value GET and PUT
            client-> last_result.message_type = UNKNOWN;
            pthread_t thread;
            thread_data* arg = (thread_data*) malloc(sizeof(thread_data));
            arg->msg = *incoming_message;
            arg->packet = incoming_packet;
            arg->cli = client;
            arg->server = server_socket;
            printf("before creating, client: %p\n", arg->cli);
            pthread_create(&thread, NULL, client_thread, arg);
            pthread_detach(thread);

        } else if (incoming_message->sequence_number == client->last_sequence_number) {
            //the reuest is finished just resend the message
            if(client->last_result.message_type != UNKNOWN){
                // printf("send 1\n");
                message return_message = client->last_result;
                send_packet(server_socket, client_socket, incoming_packet.slen, (char*)&return_message, sizeof(return_message));
            }else{// the request is executing just send the ack message
                // printf("send ack\n");
                message ack_message = {incoming_message->sequence_number, 
                incoming_message->client_id, 
                ACK, 
                incoming_message->time, 
                incoming_message->key, 
                incoming_message->value};

                send_packet(server_socket, client_socket, incoming_packet.slen, (char*) &ack_message, sizeof(ack_message));
            }
        } else {
            // Old request
            printf("Old message received.\n");
        }
    }
    close_socket(server_socket);
    return 0;
}


