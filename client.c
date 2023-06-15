#include "client.h"
#include "message.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

// initializes the RPC connection to the server


struct rpc_connection RPC_init(int src_port, int dst_port, char dst_addr[]){
    struct rpc_connection rpc;
    struct sockaddr_storage addr;
    socklen_t addrlen;
    struct timeval tv;
    gettimeofday(&tv, NULL);
    srand(tv.tv_usec);
    rpc.client_id = rand();
    rpc.recv_socket = init_socket(src_port);
    rpc.dst_addr = *(struct sockaddr*)dst_addr;
    populate_sockaddr(AF_INET, dst_port,dst_addr, &addr, &addrlen);
    rpc.dst_addr = *((struct sockaddr*)(&addr));
    rpc.dst_len = addrlen;
    rpc.seq_number = 1;
    
    return rpc;
}

message RPC_run(struct rpc_connection *rpc, message msg, enum messaget message_type){
    //printf("seq: %d\n", msg.sequence_number);
    //try sending for at most 5 times
   //printf("clinet id:::%d\n",msg.client_id);
   send_packet(rpc->recv_socket, rpc->dst_addr, rpc->dst_len, (char *)&msg, sizeof(msg));
   //fflush(stdout);
    for(int i=0; i < 5; i ++){
        struct packet_info packet = receive_packet_timeout(rpc->recv_socket, 1); // Wait for 1 second to receive the response

        //fflush(stdout);
        //recieved the message
        if(packet.recv_len != -1){
            message *return_message = malloc(sizeof(message));
            memcpy(return_message,packet.buf,sizeof(message));
            // printf("message type:%d\n",return_message.message_type);
            // printf("sequnce number:%d\n",return_message.sequence_number);
            if(return_message->message_type == ACK){
                sleep(1);
                //if ACK recount 5 times
                if(return_message->client_id!=rpc->client_id || return_message->sequence_number != msg.sequence_number){
                    continue;
                }
                send_packet(rpc->recv_socket, rpc->dst_addr, rpc->dst_len, (char *)&msg, sizeof(msg));
               i = 0;
               continue;
            }else{
                if(return_message->client_id!=rpc->client_id){
                    continue;
                }
                if(return_message->sequence_number != msg.sequence_number){
                    continue;
                }
                if(return_message->message_type != message_type){
                    continue;
                }
                return *return_message;
            }
        }else {//no message
           send_packet(rpc->recv_socket, rpc->dst_addr, rpc->dst_len, (char *)&msg, sizeof(msg));
        }
    }
    printf("no respond timeout\n");
    fflush(stdout);
    exit(1);
}

// Sleeps the server thread for a few seconds
void RPC_idle(struct rpc_connection *rpc, int time){

    //This function will initiate and block until the idle RPC is completed on the server
    // TODO 1. Create an integer variable time to represent the idle time in seconds.
    // TODO Set the data_type field of the msg struct to INT to indicate that the payload data type is an integer.

    // Create a message struct variable named msg to hold the message data.
    message msg;
    msg.message_type = IDEL_CALL;
    msg.sequence_number = rpc->seq_number;
    msg.client_id = rpc->client_id;
    // Use the snprintf function to format the integer time as a string and store it in the payload 
    // field of the msgpayload struct.
    msg.time = time;
    msg.key = 0;
    msg.value = 0;
    /*Maddie changed this part*/
    // Call the send_packet function with the rpc struct's recv_socket, dst_addr, dst_len,
    // and a pointer to the msg struct as arguments to send the message to the remote server.
    // arguments to wait for the response message for time seconds.
    // Once the response message is received, the RPC_idle function will unblock and return.
    // If the response is not
    //  received within time seconds, receive_packet_timeout will return
    // and the function will also return.
    RPC_run(rpc, msg, IDEL_RETURN);

    //  Increment the seq_number field of the rpc struct to generate a unique sequence number for the message.
    msg.sequence_number = rpc->seq_number++;
}

int RPC_get(struct rpc_connection *rpc, int key){

    // make message first
    //Create a message struct with message_type set to 1 to represent a "get" request.
    message get_msg = {rpc->seq_number, rpc->client_id, GET_CALL, 0, key, 0};

    /*Maddie changed this part*/
    // Call the send_packet function with the recv_socket from the rpc_connection struct,
    // the dst_addr and dst_len fields from the rpc_connection struct, and the message struct as
    // the payload.
    // Receive response from server
    // Call the receive_packet_timeout function with the recv_socket from the rpc_connection struct and a
    // timeout value to wait for the response from the server.
    message response_msg = RPC_run(rpc, get_msg,GET_RETURN);
    
    // Check that the recv_len field of the returned packet_info struct is greater than 0 to ensure that a
    // valid response was received.
    

    // Check that sequence number matches
    if (response_msg.sequence_number != get_msg.sequence_number) {
        return -1; // return -1 to indicate error
    } // Call the receive_packet_timeout function with the rpc struct's recv_socket and time as


    // Check that the message_type field of the received message struct is 1 to ensure that this is a "get" response.
    if (response_msg.message_type != GET_RETURN) {
        return -1; // return -1 to indicate error
    }

    // Set the sequence_number field of the message struct to rpc->seq_number,
    // where rpc is a pointer to the rpc_connection struct and rpc->seq_number is the current
    // sequence number.
    // Increment the rpc->seq_number field of the rpc_connection struct to ensure that each
    // message has a unique sequence number.
    get_msg.sequence_number = rpc->seq_number++;

    return response_msg.value;

}



// sets the value of a key on the server store
int RPC_put(struct rpc_connection *rpc, int key, int value) {
    // Create a message struct variable named msg to hold the message data.
    message msg;
    msg.client_id = rpc-> client_id;
    msg.sequence_number = rpc->seq_number++; // Increment the seq_number field of the rpc struct to generate a unique sequence number for the message.
    msg.message_type = PUT_CALL; // Set the message_type field of the msg struct to PUT to indicate that this is a put message.

    // Set the payload field of the msg struct to hold the key-value pair using snprintf
    msg.key = key;
    msg.value = value;
    msg.time = 0;
    // Call the send_packet function with the rpc struct's recv_socket, dst_addr, dst_len, and a pointer to the msg struct as arguments to send the message to the remote server.
    // Call the receive_packet function with the rpc struct's recv_socket, and a pointer to the msg struct as arguments to wait for the response message.
    RPC_run(rpc, msg,PUT_RETURN);

    // Parse the response message and return the result code
    return 0;
}


void RPC_close(struct rpc_connection *rpc){
   close_socket(rpc->recv_socket);
}
   