#ifndef CORE_MQTT_CONFIG_H_
#define CORE_MQTT_CONFIG_H_

#include <stdio.h>

/* Maximum number of MQTT PINGREQ messages that can be sent without receiving 
 * a PINGRESP before the client considers the connection broken. */
#define MQTT_PING_RETRY_LIMIT              ( 3U )

/* The maximum size of the header parsing buffer. */
#define MQTT_MAX_CONNACK_RECEIVE_RETRY_COUNT ( 5U )

#define MQTT_VERSION_5_ENABLED    ( 1 )

// Maps coreMQTT internal logs to the Raspberry Pi Pico printf

#define LogError( message )    printf( "[ERROR] " ); printf message; printf( "\n" )
#define LogWarn( message )     printf( "[WARN]  " ); printf message; printf( "\n" )
#define LogInfo( message )     printf( "[INFO]  " ); printf message; printf( "\n" )
#define LogDebug( message )    printf( "[DEBUG] " ); printf message; printf( "\n" )

#endif /* ifndef CORE_MQTT_CONFIG_H_ */
