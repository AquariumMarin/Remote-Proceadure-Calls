enum messaget{GET_CALL, PUT_CALL, IDEL_CALL, GET_RETURN, PUT_RETURN, IDEL_RETURN, ACK,UNKNOWN};

typedef struct{
    int sequence_number;    //the sequence_number
    int client_id;  //the client id
    enum messaget message_type;   //the message type: get 1, put 2, 
    int time;
    int key;
    int value;
}message;

 