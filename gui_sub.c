/*
 * Eclipse Mosquitto: 
 * - https://mosquitto.org/
 * - https://github.com/eclipse/mosquitto
 * MQTT test server: 
 * - https://test.mosquitto.org/
 */

//////////////////////////////////////////////////////////////////////
// Install software packages for Ubuntu/Debian 
// $ sudo apt install mosquitto-clients libmosquitto-dev 

// Compile the source code file
// $ gcc mosq_sub.c -o mosq_sub -lmosquitto -lpthread

// Run the executable file to subscribe to the topic on the MQTT server
// $ ./mosq_sub

// Publish a message to the MQTT server
// $ mosquitto_pub -h test.mosquitto.org -p 1883 -t '/test321' -m 'hello'
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <mosquitto.h>

#define MQTT_SERVER     "localhost"
#define MQTT_PORT       1883
#define KEEP_ALIVE      60
#define MQTT_QOS_LEVEL  2
#define MQTT_SUB_TOPIC  "/torrz/#"
#define MSG_MAX_SIZE    512

bool is_connected = false;
bool is_subscribed = false;
bool is_running = true;

// forward declarations
void on_connect_cb( 
    struct mosquitto *mosq, void *userdata, int result );
    
void on_subscribe_cb(
    struct mosquitto *mosq, void *userdata, int mid, 
    int qos_count, const int *granted_qos );

void on_message_cb( 
    struct mosquitto *mosq, void *userdata, 
    const struct mosquitto_message *msg );

int main( int argc, char *argv[]) {
	bool clean_session = true;
    struct mosquitto *mosq = NULL;

    printf( "Mosquitto - MQTT Client Demo...\n" );
    
    // Initialize the libmosquito routines
    mosquitto_lib_init();
    
    // Create a mosquitto client
    mosq = mosquitto_new( NULL, clean_session, NULL );
    if( !mosq ) {
        printf( "Create client failed..\n");
        mosquitto_lib_cleanup( );
        return 1;
    }
    
    // mosquitto_username_pw_set( mosq, "user","password" );
    
    // Set callback functions if necessary
    mosquitto_connect_callback_set( mosq, on_connect_cb );
    mosquitto_subscribe_callback_set( mosq, on_subscribe_cb );
    mosquitto_message_callback_set( mosq, on_message_cb );

    // Connect to server
    if (mosquitto_connect(mosq, MQTT_SERVER, MQTT_PORT, KEEP_ALIVE)) {
        fprintf( stderr, "Unable to connect.\n" );
        return 1;
    }
    
    int result;
    result = mosquitto_loop_start(mosq); 
    if (result != MOSQ_ERR_SUCCESS) {
        printf("mosquitto loop error\n");
        return 1;
    }

    while (!is_connected) {
		usleep(100000);
    }
    
    mosquitto_subscribe( mosq, NULL, MQTT_SUB_TOPIC, MQTT_QOS_LEVEL );
    
    while (!is_subscribed) {
		usleep(100000);
    }
    
    while(is_running) {
        usleep(100000);
    }
  
    
    if (mosq) {
       mosquitto_destroy( mosq );
    }
    mosquitto_lib_cleanup();
	return 0;
}

void on_connect_cb(
    struct mosquitto *mosq, void *userdata, int result )
{
    if (result!=0) {
        fprintf( stderr, "Connect failed\n" );
    } else{
		is_connected = true;
        printf( "Connect OK\n" );
    }
}
    
void on_subscribe_cb(
    struct mosquitto *mosq, void *userdata, int mid, 
    int qos_count, const int *granted_qos )
{
    is_subscribed = true;
	printf( "Subscribe OK, (QOS=%d)\n", granted_qos[0] );
}

void on_message_cb( 
    struct mosquitto *mosq, void *userdata, 
    const struct mosquitto_message *msg )
{
    FILE *fp;
    fp = fopen("data.txt","a");

	int len = msg->payloadlen;
    printf( "Message received: %d byte(s)\n", len );
    if ( len > 0 )
    {
        printf( "topic='%s', msg='%s'\n",msg->topic, (char *)msg->payload );
        fprintf(fp,"%s\n",(char *)msg->payload);
    }
    else { // the received message is empty
		is_running = false;
	}
    fclose(fp);
    fflush( stdout ); // flush the standard output
}
//////////////////////////////////////////////////////////////////////
