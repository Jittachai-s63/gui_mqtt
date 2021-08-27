// $ sudo apt-get install libgtk-3-dev
/*
 - gcc include flags: `pkg-config --cflags gtk+-3.0`
 - gcc library flags: `pkg-config --libs gtk+-3.0`

 $ gcc -Wall -g gui.c -o gui -I/usr/include/json-c -ljson-c -lmosquitto -lpthread \
    `pkg-config --cflags gtk+-3.0` \
    `pkg-config --libs gtk+-3.0`
*/ 
////////////////////////////////////////////////////////////////
#include <inttypes.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <mosquitto.h>
#include <time.h>

#include "json.h"
#include "json_inttypes.h"
#include "json_object.h"
#include "json_tokener.h"

#define MQTT_SERVER     "localhost"
#define MQTT_PORT       1883
#define KEEP_ALIVE      60
#define MQTT_PUB_TOPIC  "/torrz"
#define MSG_MAX_SIZE    512

GtkWidget *window;
GtkWidget *box;
GtkWidget *button1, *button2;
GtkWidget *entry1, *entry2,*entry3, *entry4;
GtkWidget *label1, *label2,*label3, *label4;
bool clean_session = true;
struct mosquitto *mosq = NULL;
bool is_connected = false;
int result;

void on_connect_cb( struct mosquitto *mosq, void *userdata, int result );

static void button1_callback( GtkWidget *widget, gpointer data ) {
    const gchar *entry1_text;
    const gchar *entry2_text;
    const gchar *entry3_text;
    const gchar *entry4_text;
    const char *json_str;
    json_object *json_obj;

	entry1_text = gtk_entry_get_text (GTK_ENTRY (entry1));
    entry2_text = gtk_entry_get_text (GTK_ENTRY (entry2));
    entry3_text = gtk_entry_get_text (GTK_ENTRY (entry3));
    entry4_text = gtk_entry_get_text (GTK_ENTRY (entry4));
    //g_print("to = %s\nmes = %s\n",entry1_text,entry2_text);

    json_obj = json_tokener_parse( "{ }" );
    json_object_object_add( json_obj, entry1_text, 
        json_object_new_string( entry2_text ) );
    json_object_object_add( json_obj, entry3_text, 
        json_object_new_string( entry4_text ) );
    
    
    json_str = json_object_to_json_string_ext( 
        json_obj, JSON_C_TO_STRING_PRETTY );
    printf( "%s\n", json_str );

    result = mosquitto_publish( mosq, NULL, MQTT_PUB_TOPIC, strlen(json_str), json_str, 0, 0 );
    if (result != 0) {
		fprintf( stderr, "MQTT publish error: %d\n", result );
	}
}

static void button2_callback( GtkWidget *widget, gpointer data ) {
	g_print( "Exit...\n" );
}

static void activate( GtkApplication *app, gpointer user_data ) {
    

	// create a GTK application window (main window)
	window = gtk_application_window_new( app );
	// set the title of the application window
	gtk_window_set_title( GTK_WINDOW(window), "GTK Demo" );
	// set the default window size (width x height) in pixels
	gtk_window_set_default_size( GTK_WINDOW(window), 200, 160 );
	// set window position
	gtk_window_set_position( GTK_WINDOW(window), GTK_WIN_POS_CENTER );
	// set window border width
	gtk_container_set_border_width( GTK_CONTAINER(window), 4 );

	// create a box for containing buttons (vertical orientation)
	// see: https://www.manpagez.com/html/gtk3/gtk3-3.24.14/GtkBox.php
	box = gtk_box_new( 
		GTK_ORIENTATION_VERTICAL, 0 /*spacing*/ );
	// add the box to the application window
	gtk_container_add( GTK_CONTAINER(window), box );
  
	// create two buttons: button1 and button2
	// see: https://www.manpagez.com/html/gtk3/gtk3-3.24.14/GtkButton.php

    label1 = gtk_label_new ("key1");
    label2 = gtk_label_new ("val1");
    label3 = gtk_label_new ("key2");
    label4 = gtk_label_new ("val2");

    entry1 = gtk_entry_new ();
    entry2 = gtk_entry_new ();
    entry3 = gtk_entry_new ();
    entry4 = gtk_entry_new ();
    
    

	// create a button for enable/disable counter
	button1 = gtk_button_new_with_label( "publish" );
	g_signal_connect( button1, "clicked",
		G_CALLBACK(button1_callback), button1 );

	// create a button for exit 
	button2 = gtk_button_new_with_label( "Exit" );
	g_signal_connect( button2, "clicked",
		G_CALLBACK(button2_callback), NULL );
	g_signal_connect_swapped( button2, "clicked", 
		G_CALLBACK( gtk_widget_destroy), window );
 
	// add the buttons to the box 
    gtk_box_pack_start( GTK_BOX(box), label1,
		TRUE /*exapnd*/, TRUE /*fill*/, 4 /*padding*/ );
    gtk_box_pack_start( GTK_BOX(box), entry1,
		TRUE /*exapnd*/, TRUE /*fill*/, 4 /*padding*/ );
    gtk_box_pack_start( GTK_BOX(box), label2,
		TRUE /*exapnd*/, TRUE /*fill*/, 4 /*padding*/ );
    gtk_box_pack_start( GTK_BOX(box), entry2,
		TRUE /*exapnd*/, TRUE /*fill*/, 4 /*padding*/ );
    gtk_box_pack_start( GTK_BOX(box), label3,
		TRUE /*exapnd*/, TRUE /*fill*/, 4 /*padding*/ );
    gtk_box_pack_start( GTK_BOX(box), entry3,
		TRUE /*exapnd*/, TRUE /*fill*/, 4 /*padding*/ );
    gtk_box_pack_start( GTK_BOX(box), label4,
		TRUE /*exapnd*/, TRUE /*fill*/, 4 /*padding*/ );
    gtk_box_pack_start( GTK_BOX(box), entry4,
		TRUE /*exapnd*/, TRUE /*fill*/, 4 /*padding*/ );
	gtk_box_pack_start( GTK_BOX(box), button1,
		TRUE /*exapnd*/, TRUE /*fill*/, 4 /*padding*/ );
	gtk_box_pack_start( GTK_BOX(box), button2, 
		TRUE /*exapnd*/, TRUE /*fill*/, 4 /*padding*/ );
  
	// make the window and its children widgets visible
	gtk_widget_show_all( window );
}

int main( int argc,char **argv ) {

    /////////////////////////////////////////////////////////////////////////

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
    
    mosquitto_connect_callback_set( mosq, on_connect_cb );

    // Connect to server
    if (mosquitto_connect(mosq, MQTT_SERVER, MQTT_PORT, KEEP_ALIVE)) {
        fprintf( stderr, "Unable to connect.\n" );
        return 1;
    }

    result = mosquitto_loop_start(mosq); 
    if (result != MOSQ_ERR_SUCCESS) {
        printf("mosquitto loop error\n");
        return 1;
    }

    while (!is_connected) {
		usleep(100000); // sleep for 0.1 seconds
    }

    GtkApplication *app;
	app = gtk_application_new( NULL, G_APPLICATION_FLAGS_NONE );

	//g_timeout_add_seconds( 1 /*sec*/, G_SOURCE_FUNC(timeout), NULL );
  
	g_signal_connect( app, "activate", 
		G_CALLBACK(activate), NULL );
	// start the application main loop (blocking call)
	g_application_run( G_APPLICATION(app), argc, argv );
	// decrease the reference count to the object
	g_object_unref( app );

	return 0;
}
////////////////////////////////////////////////////////////////

void on_connect_cb( struct mosquitto *mosq, void *userdata, int result ) {
    if (result!=0) {
        fprintf( stderr, "Connect failed\n" );
    } else{
		is_connected = true;
        printf( "Connect OK\n" );
    }
}