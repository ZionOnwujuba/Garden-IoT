#pragma GCC optimize("O2")
extern "C" {
    #include "FreeRTOS.h"
    #include "task.h"
    #include "rs485.h"
    #include "dht22.h"
    #include "lcd.h"
    #include "light_sensor.h"
    #include "constants.h"

    #include "pico/stdlib.h"
    #include "pico/cyw43_arch.h"
    #include "pico/time.h"
    #include <queue.h>
    #include "key_certs.h"

}
#include <stdio.h>


#undef STATIC 

// ExecuTorch Runtime Includes
#include "executorch/extension/data_loader/buffer_data_loader.h"
#include "executorch/runtime/executor/program.h"
#include "executorch/runtime/platform/platform.h"
#include "executorch/runtime/core/exec_aten/exec_aten.h"
#include "executorch/runtime/core/exec_aten/util/tensor_util.h"

extern "C" {
    #ifdef write
    #undef write
    #endif
    #ifdef read
    #undef read
    #endif

    #include "core_mqtt.h"
    #include "core_mqtt_serializer.h"
    #include "mbedtls/net_sockets.h"
    #include "mbedtls/ssl.h"
    #include "mbedtls/entropy.h"
    #include "mbedtls/ctr_drbg.h"
    #include "mbedtls/platform_time.h"
    #include "lwip/apps/sntp.h"

    // Prototype definitions for tasks
    void vInferenceTask(void *pvParameters);
    void led_task(void *pvParameters);
    void data_aquisition_task(void *pvParameters);

    // Basic low-level stub wrappers to satisfy POSIX bindings on bare-metal systems
    void _exit(int status) { while(1); }
    int _getpid(void) { return 1; }
    int _kill(int pid, int sig) { return -1; }

    // AWS Credentials Strings (TODO: Add to .gitignore)
    extern const char rootCA[] = ROOTCA;
    extern const char deviceCert[]= DEVICECERT;
    extern const char privateKey[]= PRIVATEKEY;
    
     #include "hardware/rosc.h"

    /* Hardware entropy generator using RP2040 Ring Oscillator (ROSC) jitter */
    /*
    int mbedtls_hardware_poll(void *data, unsigned char *output, size_t len, size_t *olen) {
        (void)data;
        for (size_t i = 0; i < len; i++) {
            uint8_t byte = 0;
            for (int bit = 0; bit < 8; bit++) {
                byte = (byte << 1) | (rosc_hw->randombit & 1);
            }
            output[i] = byte;
        }
        *olen = len;
        return 0;
    }
    */
    #include "psa/crypto.h"

    psa_status_t mbedtls_psa_external_get_random(
    mbedtls_psa_external_random_context_t *context,
    uint8_t *output, size_t output_size, size_t *output_length)
    {
    (void)context;

    for (size_t i = 0; i < output_size; i++) {
        uint8_t byte = 0;
        for (int bit = 0; bit < 8; bit++) {
            byte = (byte << 1) | (rosc_hw->randombit & 1);
        }
        output[i] = byte;
    }

    *output_length = output_size;
    return PSA_SUCCESS;
    }
    

    #include "lwip/sockets.h"
    #include "lwip/netdb.h"
    #include "lwip/inet.h"

    // Define mbedTLS error codes if net_sockets.h is omitted
    #ifndef MBEDTLS_ERR_NET_SEND_FAILED
    #define MBEDTLS_ERR_NET_SEND_FAILED -0x004E
    #define MBEDTLS_ERR_NET_RECV_FAILED -0x004C
    #endif

    /* Custom mbedTLS BIO Send Callback using lwIP */
    static int lwip_mbedtls_bio_send(void *ctx, const unsigned char *buf, size_t len) {
        int fd = *(reinterpret_cast<int*>(ctx));
        int ret = lwip_send(fd, buf, len, 0);
        if (ret < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                return MBEDTLS_ERR_SSL_WANT_WRITE;
            }
            return MBEDTLS_ERR_NET_SEND_FAILED;
        }
        return ret; // Return bytes sent
    }

    /* Custom mbedTLS BIO Recv Callback using lwIP */
    static int lwip_mbedtls_bio_recv(void *ctx, unsigned char *buf, size_t len) {
        int fd = *(reinterpret_cast<int*>(ctx));
        int ret = lwip_recv(fd, buf, len, 0);
        if (ret < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                return MBEDTLS_ERR_SSL_WANT_READ;
            }
            return MBEDTLS_ERR_NET_RECV_FAILED;
        }
        return ret; // Return bytes received
    }
}

#include "gini_model.h" 

using namespace ::executorch::runtime;
using namespace ::executorch::extension;
using executorch::aten::Tensor;

// Define memory arena sizes 
#define INFERENCE_TASK_STACK_SIZE   (8 * 1024) / sizeof(StackType_t)
#define MEMORY_POOL_SIZE            (40 * 1024)

// TODO: Add to gitignore
const char* ssid = SSID;
const char* password = WIFIPWD;
const char* awsEndpoint = AWSENDPOINT;
const int awsPort = AWSPORT;

const char* pubTopic = PUBTOPIC; 
const char* subTopic = SUBTOPIC;
const char* clientID = CLIENTID;



struct NetworkContext {
    mbedtls_ssl_context * pSslContext;
};

mbedtls_ms_time_t mbedtls_ms_time(void) {
    return (mbedtls_ms_time_t)(to_ms_since_boot(get_absolute_time()));
}

// coreMQTT and mbedTLS structural variables
static MQTTContext_t xMqttContext;
static NetworkContext_t xNetworkContext;
static TransportInterface_t xTransport;
static mbedtls_net_context xNetContext;
static mbedtls_ssl_context xSslContext; // TLS connection state
static mbedtls_ssl_config xSslConfig; // TLS configuration (ciphers, TLS version, etc)
static mbedtls_entropy_context xEntropyContext; // RNG for session keys, 
                                                // gathers raw randomness from hardware RNG hook
static mbedtls_ctr_drbg_context xCtrDrbgContext; // Cryptographically secure PRNG that is seeded 
                                                // from entropy and prouces random numbers TLS uses
static mbedtls_x509_crt xRootCaCert;
static mbedtls_x509_crt xClientCert;
static mbedtls_pk_context xPrivKey;


#define SENSOR_INPUT_COUNT 8
typedef float sens_package[SENSOR_INPUT_COUNT];
typedef struct {
    float curr_sens_package[SENSOR_INPUT_COUNT];
    float model_pred;
} final_output_package;

void publishSensorData(final_output_package packageToPublish);

/*
0: soil_tempF
1: soil_tempC
2: soil_moisture
3: soil_ph
4: ambient_tempF
5: ambient_tempC
6: ambient_hum
7: lux
*/

static QueueHandle_t xSensorQueue = NULL;
static QueueHandle_t xModelQueue = NULL;
static QueueHandle_t xLCDQueue = NULL; // Change to Mutex?

TaskHandle_t Data_Aqu_Task_Handle;
TaskHandle_t Model_Task_Handle;
TaskHandle_t LCD_BAT_WIFI_Task_Handle;
TaskHandle_t AWS_IOT_Task_Handle;
TaskHandle_t xMonitorTaskHandle;
void led_task(void *pvParameters) 
{   
    while (1){
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        vTaskDelay(pdMS_TO_TICKS(250));

        // Turn the Pico W LED off
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        vTaskDelay(pdMS_TO_TICKS(250));
        
    }
}

/*
When coreMQTT expects an incoming message 
(or an acknowledgment packet from AWS), it calls this 
function to read a specific number of bytes (bytesToRead) 
into a buffer.
    It passes the request down to mbedtls_ssl_read, 
    which decrypts data arriving over the underlying TCP socket.    
        If mbedTLS returns MBEDTLS_ERR_SSL_WANT_READ, it means "
        the network socket is empty right now, try again later." 
        The error is caught and returns 0 bytes so the 
        system can keep running other operations.

*/
// Network wrappers mapping coreMQTT I/O transactions to mbedTLS routines
int32_t transport_recv(NetworkContext_t * pNetworkContext, void * pBuffer, size_t bytesToRead) {
    int iResult = mbedtls_ssl_read(&xSslContext, reinterpret_cast<unsigned char*>(pBuffer), bytesToRead);
    if (iResult == MBEDTLS_ERR_SSL_WANT_READ) return 0;
    return iResult;
}

/*
Whenever data is published or send a heartbeat ping, 
coreMQTT uses this to push raw bytes out to the cloud.
    It takes the unencrypted MQTT packet buffer (pBuffer) 
    and hands it to mbedtls_ssl_write. mbedTLS encrypts the 
    payload using session keys and puts it directly 
    into the Pico W’s TCP network transmission buffer.
*/
int32_t transport_send(NetworkContext_t * pNetworkContext, const void * pBuffer, size_t bytesToSend) {
    int iResult = mbedtls_ssl_write(&xSslContext, reinterpret_cast<const unsigned char*>(pBuffer), bytesToSend);
    if (iResult == MBEDTLS_ERR_SSL_WANT_WRITE) return 0;
    return iResult;
}

/*
Returns the number of milliseconds that have passed since the Pico booted.
    MQTT connections require a strict "Keep-Alive" window (typically 60 seconds). 
    If the device stays silent for too long, AWS will drop the connection. 
    coreMQTT calls this function internally to calculate exactly how much 
    time has passed since the last transmission so it knows exactly when 
    to send an automated "PINGREQ" (Ping Request) packet.
*/
uint32_t prvGetTimeMs(void) {
    return to_ms_since_boot(get_absolute_time());
}

/*
Mailbox listener triggering automatically whenever AWS sends a cloud command 
down to Pico.
    The incoming packet's header is checked via bitwise evaluation 
    (pPacketInfo->type & 0xF0). If the packet type matches an 
    MQTT_PACKET_TYPE_PUBLISH, it decodes the payload metadata 
    (pxPublishInfo) and prints the targeted topic name and message details
*/
bool prvEventCallback(MQTTContext_t * pMqttContext, 
                      MQTTPacketInfo_t * pPacketInfo, 
                      MQTTDeserializedInfo_t * pDeserializedInfo, 
                      MQTTSuccessFailReasonCode_t * pReasonCode, 
                      MQTTPropBuilder_t * pPropsBuilder, 
                      MQTTPropBuilder_t * pOutPropsBuilder) {
                      
    if ((pPacketInfo->type & 0xF0) == MQTT_PACKET_TYPE_PUBLISH) {
        MQTTPublishInfo_t * pxPublishInfo = pDeserializedInfo->pPublishInfo;
        printf("Message on topic: %.*s\n", (int)pxPublishInfo->topicNameLength, pxPublishInfo->pTopicName);
    }
    return true; // Return true to signal successful payload processing
}

bool connectToAWS() {
    int iResult;
/*
    Explicitly clearing out the data blocks allocated for things 
    like random number engines (entropy / ctr_drbg) and X.509 
    certificate formats to eliminate memory remnants from previous boots.
    (mbedtls_net_init is removed as the LwIP socket is managed directly).
    */
    mbedtls_ssl_init(&xSslContext); 
    mbedtls_ssl_config_init(&xSslConfig);
    mbedtls_entropy_init(&xEntropyContext);
    mbedtls_ctr_drbg_init(&xCtrDrbgContext);
    mbedtls_x509_crt_init(&xRootCaCert);
    mbedtls_x509_crt_init(&xClientCert);
    mbedtls_pk_init(&xPrivKey);

    /*
    Human-readable certificate text headers are processed into complex 
    cryptographic objects in RAM. Once verified, the standard TCP layer 
    is initialized by calling lwip_connect. 
        This establishes a basic, raw connection over Port 8883, but it 
        is completely unencrypted at this exact moment.
    */
    mbedtls_ctr_drbg_seed(&xCtrDrbgContext, mbedtls_entropy_func, &xEntropyContext, NULL, 0); // feeds hardware RNG entropy into PRNG, 
                                                                                            // mbedtls_ctr_drbg_random is now the N random bytes function
    mbedtls_x509_crt_parse(&xRootCaCert, (const unsigned char *)rootCA, strlen(rootCA) + 1); // Passing certs, length (including null terminator)
    mbedtls_x509_crt_parse(&xClientCert, (const unsigned char *)deviceCert, strlen(deviceCert) + 1);
    mbedtls_pk_parse_key(&xPrivKey, (const unsigned char *)privateKey, strlen(privateKey) + 1, NULL, 0, mbedtls_ctr_drbg_random, &xCtrDrbgContext);

    printf("Connecting TCP socket to AWS...\n");
    /*
    Creates a socket (communication endpoint). AF_INET = IPv4, SOCK_STREAM = TCP (byte ordered stream 
    over UDP's fire and forget). Returns a file description like integer used to refer to this connection 
    in subsequent calls
    */
    int sock_fd = lwip_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock_fd < 0) {
        printf("Failed to create LwIP socket: %d\n", sock_fd);
        return false;
    }

    /*
    awsEndpoint is the human-readable hostname, the function asks a DNS server what
    IP address the name poinst to and gets sent back a hostent struct
    containing one or more IPs
    */
    struct hostent *pHostEntry = lwip_gethostbyname(awsEndpoint);
    if (pHostEntry == NULL || pHostEntry->h_addr_list[0] == NULL) {
        printf("DNS resolution failed for %s\n", awsEndpoint);
        lwip_close(sock_fd);
        return false;
    }

    /*
    Builds destination address struct that connect() needs (address family, port, and IP).
    htons() = host to network short
        Since the endianness is dependent on CPU architectures and network protocols use big ending, 
        this convers the port number into that standard form.
    */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(awsPort); // AWS IoT MQTT port
    memcpy(&server_addr.sin_addr, pHostEntry->h_addr_list[0], sizeof(server_addr.sin_addr));

    printf("Resolved %s -> %s\n", awsEndpoint, inet_ntoa(server_addr.sin_addr));

    // Connect via lwIP directly
    /* TCP (Transmission Control Protocol) three-way handshake
    Synchronize(SYN)
        Client sends packet with SYN flags set and a starting sequence number
        to ask server for connection
    Synchronize-Acknowledgment(SYN-ACK):
        Server replies with a packet that has both SYN and ACK flags set, 
        acknowledging the client's request and shares its own starting sequence
        number
    ACK (Acknowledgment)
        Client sends back an ACK packet to confirm the server's message.
        Connection is open, data transfer can begin albeit, with no encryption
    */
    if (lwip_connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        printf("Failed to connect LwIP socket\n");
        lwip_close(sock_fd);
        return false;
    }

    /*
    Mutual Authentication (mTLS): AWS IoT Core requires both parties to 
    verify each other. The Pico checks the server's certificate against the 
    rootCA to prove it is talking to a real AWS server.
        AWS checks the Pico's deviceCert and privateKey to prove this 
        specific hardware node is authorized. The while loop runs the 
        complex math required for the handshake. Once complete, an 
        encrypted tunnel is established.
    */

    // TLS Config
    mbedtls_ssl_config_defaults(&xSslConfig, MBEDTLS_SSL_IS_CLIENT, MBEDTLS_SSL_TRANSPORT_STREAM, MBEDTLS_SSL_PRESET_DEFAULT); // Config as a client
    mbedtls_ssl_conf_authmode(&xSslConfig, MBEDTLS_SSL_VERIFY_REQUIRED); // Rejects the connection if server's cert doesn't validate
    mbedtls_ssl_conf_ca_chain(&xSslConfig, &xRootCaCert, NULL); // Setting trusted CA
    mbedtls_ssl_conf_rng(&xSslConfig, mbedtls_ctr_drbg_random, &xCtrDrbgContext); // Setting RNG
    mbedtls_ssl_conf_own_cert(&xSslConfig, &xClientCert, &xPrivKey); // Cert+key to show to server

    mbedtls_ssl_setup(&xSslContext, &xSslConfig); // Applies the SSL config (the one set above) to the connection context
    mbedtls_ssl_set_hostname(&xSslContext, awsEndpoint); // Used for Server Name Indication (telling the server which hostname 
                                                         // you're trying to reach since one IP can host many TLS domains) and 
                                                         // it's what the certificate hostname-matching check compares against during verification
    
    // Bind the raw socket descriptor & custom LwIP send/recv callbacks to Mbed TLS
    // Since mbedtls is transport-agnostic (can run over a socket, a serial port, 
    // etc, BIO means Basic I/O) we use lwip_mbedtls_bio_send/lwip_mbedtls_bio_recv 
    // so whenever mbedtls needs to push or pull bytes, it calls the custom functions 
    // which talk to the socket
    mbedtls_ssl_set_bio(&xSslContext, 
                        &sock_fd, 
                        lwip_mbedtls_bio_send, 
                        lwip_mbedtls_bio_recv, 
                        NULL);
                        

    printf("Performing TLS handshake...\n");
    // Drives the TLS handshake (ClientHello, ServerHello, certificate exchange, key agreement), 
    // looping through the multi-round and non-blocking friendly trip
    // mbedtls returns WANT_READ/WANT_WRITE to mean it needs more data or can't send at the 
    // moment and to try again respectively, which is normal and expect. However, any other return
    // means the handshake failed
    while ((iResult = mbedtls_ssl_handshake(&xSslContext)) != 0) {
        if (iResult != MBEDTLS_ERR_SSL_WANT_READ && iResult != MBEDTLS_ERR_SSL_WANT_WRITE) {
            printf("TLS Handshake failed: -0x%x\n", -iResult);
            lwip_close(sock_fd);
            return false;
        }
    }
    printf("TLS Handshake successful!\n");

    // Initialize coreMQTT Context Mapping
    // With the socket encrypted, this layer puts the MQTT pub/sub messaging protocol over it
    // Since coreMQTT does not know anyhting about TLS or sockets, the function pointers
    // transport_send/recv so it can send/recieve MQTT packets through the TLS tunnel
    xTransport.pNetworkContext = &xNetworkContext;
    xTransport.recv = transport_recv;
    xTransport.send = transport_send;
    /*
    Application Layer Binding: Now that the secure tunnel is up, 
    a standard MQTT connection packet (MQTTConnectInfo_t) is packed
    and pass it through the tunnel using MQTT_Connect. 
        Once AWS approves the application-layer link, an 
        MQTT_Subscribe packet is sent to establish a routing listener 
        for incoming cloud command streams
    */
    static uint8_t ucNetworkBuffer[2048];
    MQTTFixedBuffer_t xFixedBuffer = { .pBuffer = ucNetworkBuffer, .size = sizeof(ucNetworkBuffer) };
    
    // Sets up MQTT client state (which transport to use, time function to track timeouts, a 
    // callback for unsolicited incoming messages, and a fixed buffer to serialize/parse 
    // packets into since there is no dynamic allocation)
    MQTT_Init(&xMqttContext, &xTransport, prvGetTimeMs, prvEventCallback, &xFixedBuffer);

    // Sends the MQTT Connect packet (application layer handshake where the broker is told 
    // the client ID and the connection preferences and the server accepts or rejects the 
    // device), a TLS handshake can succeed while MQTT fails (bad client ID or policy denial)
    MQTTConnectInfo_t xConnectInfo = {};
    xConnectInfo.cleanSession = true;
    xConnectInfo.pClientIdentifier = clientID;
    xConnectInfo.clientIdentifierLength = static_cast<uint16_t>(strlen(clientID));
    xConnectInfo.keepAliveSeconds = 60;

    printf("Connecting to AWS IoT Core over MQTT...");
    bool bSessionPresent;
    MQTTStatus_t xMqttStatus = MQTT_Connect(&xMqttContext, &xConnectInfo, NULL, 5000, &bSessionPresent, NULL, NULL);
    
    // Only if MQTT-level connect succeeds does the device subscribe to the command topic and then 
    // report success to vAWSIotTask
    if (xMqttStatus == MQTTSuccess) {
        printf(" connected!\n");
        
        MQTTSubscribeInfo_t xSubInfo = {};
        xSubInfo.qos = MQTTQoS1;
        xSubInfo.pTopicFilter = subTopic;
        xSubInfo.topicFilterLength = static_cast<uint16_t>(strlen(subTopic));
        
        MQTT_Subscribe(&xMqttContext, &xSubInfo, 1, 1, NULL);
        return true;
    } else {
        printf(" failed.\n");
        return false;
    }
}

/*
Packages telemetry variables into an industry-standard JSON text 
string using safe C memory bounds (snprintf).
    It wraps the JSON text inside an MQTTPublishInfo_t container 
    and sets the parameter .qos = MQTTQoS1 (Quality of Service Level 1). 
        This ensures this message reaches AWS at least once. If the network 
        drops a packet, it waits for a confirmation block, or retries sending it.
*/

void publishSensorData(final_output_package *packageToPublish) {
    char payload[320];
    int n = snprintf(payload, sizeof(payload), 
             "{"
             "\"deviceId\":\"%s\","
             "\"soilTempF\":%.1f,"
             "\"soilTempC\":%.1f,"
             "\"soilMoisture\":%.1f,"
             "\"soilPH\":%.1f,"
             "\"ambientTempF\":%.1f,"
             "\"ambientTempC\":%.1f,"
             "\"ambientHum\":%.1f,"
             "\"lux\":%.1f,"
             "\"modelPred\":%.1f,"
             "\"timestamp\":%lu"
             "}", 
             clientID, packageToPublish->curr_sens_package[0], 
             packageToPublish->curr_sens_package[1], 
             packageToPublish->curr_sens_package[2], 
             packageToPublish->curr_sens_package[3], 
             packageToPublish->curr_sens_package[4], 
             packageToPublish->curr_sens_package[5], 
             packageToPublish->curr_sens_package[6], 
             packageToPublish->curr_sens_package[7], 
             packageToPublish->model_pred, 
             (unsigned long)prvGetTimeMs());

    if (n < 0 || (size_t)n >= sizeof(payload)) {
        printf("WARNING: payload truncated (%d bytes needed)\n", n);
    }
    MQTTPublishInfo_t xPublishInfo = {
        .qos = MQTTQoS1,
        .pTopicName = pubTopic,
        .topicNameLength = strlen(pubTopic),
        .pPayload = payload,
        .payloadLength = strlen(payload)
    };

    uint16_t packetId = MQTT_GetPacketId(&xMqttContext);
    MQTT_Publish(&xMqttContext, &xPublishInfo, packetId, NULL);
    printf("%s\n", payload);
    
}

void vAwsIotTask(void *pvParameters) {

    cyw43_arch_enable_sta_mode(); // Station mode (client mode)

    printf("Connecting to Wi-Fi...");
    // TODO: Add timeout break
    while (cyw43_arch_wifi_connect_timeout_ms(ssid, password, CYW43_AUTH_WPA2_AES_PSK, 15000)) {
        printf(".");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    printf("\nWi-Fi connected\n");
    lwip_socket_init();

    // Mandatory SNTP time synchronization to avoid TLS rejection
    /*
    Activates an SNTP (Simple Network Time Protocol) client over the internet 
    to fetch the current epoch time. 
        The while loop checks the hardware clock status. If the clock reports 
        a timestamp close to 1970, it halts progress via vTaskDelay, allowing 
        the network time client background threads to safely fetch the correct 
        year from public servers before initializing the AWS crypto libraries.
    */
    sntp_init();
    printf("Synchronizing Time...");
    // TODO: Add timeout break
    while (time(NULL) < 100000000) {
        printf(".");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    printf("\nTime Sync Complete.\n");

    bool connected = connectToAWS();

    TickType_t xLastPublishTime = xTaskGetTickCount();
    static final_output_package packageToPublish;

    UBaseType_t uxHighWaterMark;

    /* Inspect the stack high water mark on entering the task */
    uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    printf("AWS IoT Task started. Initial free stack: %lu words\n", (unsigned long)uxHighWaterMark);
    
    while (1) {
        // Run coreMQTT processing window to parse arriving commands and maintain keep-alive
        /*
        Instead of a blocking delay loop, MQTT_ProcessLoop opens a tiny 100-millisecond 
        window to check for new network packets, manage heartbeats, and handle background 
        housekeep routing.
            FreeRTOS clock ticks (xTaskGetTickCount()) are used to track time differences 
            non-blockingly. When 10 seconds pass, it triggers data publication. 
            The final vTaskDelay(pdMS_TO_TICKS(10)) is critical. It releases control 
            of the CPU back to FreeRTOS for 10 milliseconds, allowing other low-priority 
            functions or sensor drivers to execute.
        */
        if (!connected) {
            vTaskDelay(pdMS_TO_TICKS(5000));
            connected = connectToAWS();
            continue;
        }

        MQTTStatus_t status = MQTT_ProcessLoop(&xMqttContext);
        if (status != MQTTSuccess) {
            printf("MQTT_ProcessLoop failed: %d — reconnecting\n", status);
            connected = false;
            continue;
        }

        // Evaluate interval using FreeRTOS clock metrics instead of millis() math loops
        if ((xTaskGetTickCount() - xLastPublishTime) >= pdMS_TO_TICKS(10000)) {
            if (xQueueReceive(xModelQueue, &packageToPublish, 0U) == pdTRUE) {
                xLastPublishTime = xTaskGetTickCount();
                publishSensorData(&packageToPublish);
                if(xQueueSend(xLCDQueue, &packageToPublish, 0U) == pdPASS){
                    printf("Full Data Package sent to LCD Queue\n");
                } else {
                    printf("LCD Queue is Full\n");
                }
            }
        }
        uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
        
        /* Alerts if the remaining stack falls below a critical threshold (e.g., 20 words) */
        if( uxHighWaterMark < 20 ) 
        {
            printf("WARNING: AWS IOT Task stack running low! Only %lu words left.\n", (unsigned long)uxHighWaterMark);
        }

        vTaskDelay(pdMS_TO_TICKS(10)); // Yield executing cycles brief to avoid cpu saturation
    }
}

// Planned memory pool for ExecuTorch tensor allocations
static uint8_t g_inference_pool[MEMORY_POOL_SIZE] __attribute__((section(".uninitialized_data"), aligned(16)));

// Dedicated FreeRTOS inference task routine
void vInferenceTask(void *pvParameters) {
    (void)pvParameters;

    UBaseType_t uxHighWaterMark;

    /* Inspect the stack high water mark on entering the task */
    uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    printf("Inference Task started. Initial free stack: %lu words\n", (unsigned long)uxHighWaterMark);

    printf("[ExecuTorch] Starting Inference Task...\n");
    long int input_sizes[] = {1, SENSOR_INPUT_COUNT};

    // Initialize ExecuTorch data loader with the embedded .pte array
    BufferDataLoader data_loader(models_model_ptes_gini_model_pte, models_model_ptes_gini_model_pte_len);

    // Load the ExecuTorch Program structure
    Result<Program> program = Program::load(&data_loader);
    if (!program.ok()) {
        printf("[Error] Failed to load program. Code: %d\n", program.error());
        vTaskDelete(NULL);
    }

    // Initialize Memory Manager
    /*
    A MemoryAllocator used to allocate runtime structures at Method load time. 
        Things like Tensor metadata, the internal chain of instructions, 
        and other runtime state come from this.
    */
    MemoryAllocator runtime_allocator(MEMORY_POOL_SIZE, g_inference_pool);
    MemoryManager memory_manager(&runtime_allocator);

    // Load the compiled method profile (typically "forward")
    Result<Method> method = program->load_method("forward", &memory_manager);
    if (!method.ok()) {
        printf("[Error] Failed to load method 'forward'. Code: %d\n", method.error());
        vTaskDelete(NULL);
        return;
    }
    static sens_package receiving_package;


    printf("[ExecuTorch] Model initialized successfully. Running execution loop...\n");

    
    exec_aten::Tensor::DimOrderType dim_order[] = {0, 1};
    
    while (1) {
        if (xQueueReceive(xSensorQueue, &receiving_package, portMAX_DELAY) == pdTRUE) {
            
            // Instantiating the implementation block using structure pattern.
            exec_aten::TensorImpl impl(
                exec_aten::ScalarType::Float, // Tensor type
                2, // dimension count
                input_sizes, // An array or vector containing the exact length or size of each of the dimensions.
                receiving_package,
                /*
                Dim order specifies the exact physical sequence used to flatten 
                    a multi-dimensional tensor into a continuous 1D row of computer 
                    memory addresses, mapping dimensions from the slowest-changing 
                    to the fastest-changing.
                */
                dim_order
            );
            
            // Wrap it into the executable tensor target
            exec_aten::Tensor input_tensor(&impl);
            
            EValue input_evalue(input_tensor);
            
            if (method->set_input(input_evalue, 0) != Error::Ok) {
                printf("[Error] Input binding failed\n");
                continue;
            }
            
            // Run inference execution
            if (method->execute() == Error::Ok) {
                // Pull the generic evaluation block from the model output layer
                EValue output_evalue = method->get_output(0);
                if (output_evalue.isTensor()) {
                    exec_aten::Tensor output_tensor = output_evalue.toTensor();
                    // Extract raw output data pointers safely
                    float* raw_output = output_tensor.mutable_data_ptr<float>();
                    static final_output_package output;
                    output.model_pred = raw_output[0];
                    for(int i = 0; i < SENSOR_INPUT_COUNT; i++){
                        output.curr_sens_package[i] = receiving_package[i];
                    }
                    
                    if(xQueueSend(xModelQueue, &output, 0U) == pdPASS){
                        printf("Full Data Package sent to Model Queue for AWS\n");
                    } else {
                        printf("Model Queue is Full\n");
                    }
                }
            }
        }
        uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
        
        /* Alerts if the remaining stack falls below a critical threshold (e.g., 20 words) */
        if( uxHighWaterMark < 20 ) 
        {
            printf("WARNING: Inference Task stack running low! Only %lu words left.\n", (unsigned long)uxHighWaterMark);
        }
        // Yield control to lower-priority FreeRTOS tasks for 100 milliseconds
        vTaskDelay(pdMS_TO_TICKS(100));
        
    }
}

// Global initialization override needed by ExecuTorch runtime platform layers
extern "C" void et_pal_init(void) {}

void lcd_wifi_bat_task(void *pvParameters){
    // Set up standard Pico I2C block at 100kHz standard speed
    char buffer[50];
    static final_output_package receiving_output;

    UBaseType_t uxHighWaterMark;

    /* Inspect the stack high water mark on entering the task */
    uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    printf("LCD/WIFI/Bat Task started. Initial free stack: %lu words\n", (unsigned long)uxHighWaterMark);

    while(1){
        if (xQueueReceive(xLCDQueue, &receiving_output, portMAX_DELAY) == pdTRUE) {
            int status = cyw43_wifi_link_status(&cyw43_state, CYW43_ITF_STA);
            // TODO: Battery Level Calculation
            /*
            LCD Screen 0:
                0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
            0   M P : S S   W : # #                   // Model Prediction (Acronym for each status), wifi status code
            1   B : B A T %                           // Battery
            */
            /*
            LCD Screen 1:
                0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
            0   S : T E M . P F   T  E  M .  P  C    // Soil temps
            1   A : T E M . P F   T  E  M .  P  C    // Ambient temps
            */

            /*
            LCD Screen 2:
                0 1 2 3 4 5 6 7 8 9 10 11 12 13 14 15
            0   M : H U M %   P H : P  H             // Soil moisture, PH
            1   H : H U M %   L : L U  X  l  x       // Ambient humidity
            */
            lcd_set_cursor(0, 0);
            lcd_print("MP:");
            lcd_set_cursor(3, 0);
            switch (int(receiving_output.model_pred)){
                case 0:
                    lcd_print("H");
                    break;
                case 1:
                    lcd_print("MS");
                    break;
                case 2:
                    lcd_print("HH");
                    break;
                default:
                    break;         
            }
            
            // Wifi
            snprintf(buffer, sizeof(buffer), "%d", status);
            lcd_set_cursor(6, 0);
            lcd_print("W:");
            lcd_set_cursor(8, 0);
            lcd_print(buffer);

            // Battery
            /*
            snprintf(buffer, sizeof(buffer), "%d", battery_level);
            lcd_set_cursor(0, 1);
            lcd_print("B:");
            lcd_set_cursor(2, 1);
            lcd_print(buffer);
            lcd_set_cursor(5, 1);
            lcd_print("%");
            */
            vTaskDelay(pdMS_TO_TICKS(2000));
            lcd_command(LCD_CLEARDISPLAY);
            
            // Soil Temp F
            snprintf(buffer, sizeof(buffer), "%.1f", receiving_output.curr_sens_package[0]);
            lcd_set_cursor(0, 0);
            lcd_print("S:");
            lcd_set_cursor(2, 0);
            lcd_print(buffer);
            lcd_set_cursor(7, 0);
            lcd_print("F");

            // Soil Temp C
            snprintf(buffer, sizeof(buffer), "%.1f", receiving_output.curr_sens_package[1]);
            lcd_set_cursor(9, 0);
            lcd_print(buffer);
            lcd_set_cursor(15, 0);
            lcd_print("C");


            // Ambient Temp F
            snprintf(buffer, sizeof(buffer), "%.1f", receiving_output.curr_sens_package[4]);
            lcd_set_cursor(0, 1);
            lcd_print("A:");
            lcd_set_cursor(2, 1);
            lcd_print(buffer);
            lcd_set_cursor(7, 1);
            lcd_print("F");

            // Ambient Temp C
            snprintf(buffer, sizeof(buffer), "%.1f", receiving_output.curr_sens_package[5]);
            lcd_set_cursor(9, 1);
            lcd_print(buffer);
            lcd_set_cursor(15, 1);
            lcd_print("C");
            vTaskDelay(pdMS_TO_TICKS(2000));
            lcd_command(LCD_CLEARDISPLAY);

            // Soil Moisture
            snprintf(buffer, sizeof(buffer), "%.1f", receiving_output.curr_sens_package[2]);
            lcd_set_cursor(0, 0);
            lcd_print("M:");
            lcd_set_cursor(2, 0);
            lcd_print(buffer);
            lcd_set_cursor(5, 0);
            lcd_print("%");

            // Soil Ph
            snprintf(buffer, sizeof(buffer), "%.1f", receiving_output.curr_sens_package[3]);
            lcd_set_cursor(7, 0);
            lcd_print("Ph:");
            lcd_set_cursor(10, 0);
            lcd_print(buffer);


            // Ambient Humidity
            snprintf(buffer, sizeof(buffer), "%.1f", receiving_output.curr_sens_package[2]);
            lcd_set_cursor(0, 1);
            lcd_print("H:");
            lcd_set_cursor(2, 1);
            lcd_print(buffer);
            lcd_set_cursor(5, 1);
            lcd_print("%");

            // Ambient Temp C
            snprintf(buffer, sizeof(buffer), "%.1f", receiving_output.curr_sens_package[5]);
            lcd_set_cursor(7, 1);
            lcd_print("L:");
            lcd_set_cursor(9, 1);
            lcd_print(buffer);
            lcd_set_cursor(12, 1);
            lcd_print("lx");
            vTaskDelay(pdMS_TO_TICKS(2000));
            lcd_command(LCD_CLEARDISPLAY);

            cyw43_arch_poll();
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
        
        /* Alerts if the remaining stack falls below a critical threshold (e.g., 20 words) */
        if( uxHighWaterMark < 20 ) 
        {
            printf("WARNING: LCD/WIFI/Bat Task stack running low! Only %lu words left.\n", (unsigned long)uxHighWaterMark);
        }
    }
}

void data_aquisition_task(void *pvParameters){
    dht_data current_readings;
    rs485_data rsdata;
    static sens_package reading_package;
    UBaseType_t uxHighWaterMark;

    /* Inspect the stack high water mark on entering the task */
    uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
    printf("Data Aqu Task started. Initial free stack: %lu words\n", (unsigned long)uxHighWaterMark);
    while (1) {
        /*
0: soil_tempF
1: soil_tempC
2: soil_moisture
3: soil_ph
4: ambient_tempF
5: ambient_tempC
6: ambient_hum
7: lux
*/
        if (read_from_dht(&current_readings)) {
            float fahrenheit = (current_readings.temperature * 9 / 5) + 32;
            reading_package[4] = fahrenheit;
            reading_package[6] = current_readings.humidity;
            reading_package[5] = current_readings.temperature;
        } else {
            printf("Failed to read data from DHT22 (Checksum/Timeout Error).\n");
        }
        readHumiturePH(&rsdata);
        float fahrenheit_rs = (rsdata.tem * 9 / 5) + 32;
        reading_package[1] = rsdata.tem;
        reading_package[0] = fahrenheit_rs;
        reading_package[2] = rsdata.hem;
        reading_package[3] = rsdata.ph;
        reading_package[7] = bh1750_read_light(I2C_PORT_BF1750) / 1.2;
        
        if(xQueueSend(xSensorQueue, &reading_package, 0U) == pdPASS){
            printf("Full Sensor Package sent to Sensor Queue for Model\n");
        } else {
            printf("Sensor Queue is Full\n");
        }

        uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL );
        
        /* Alerts if the remaining stack falls below a critical threshold (e.g., 20 words) */
        if( uxHighWaterMark < 20 ) 
        {
            printf("WARNING: Data Aqu Task stack running low! Only %lu words left.\n", (unsigned long)uxHighWaterMark);
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
    
}

void vMonitorTask( void * pvParameters ) 
{
    UBaseType_t uxDataAquStackLeft;
    UBaseType_t uxInferenceStackLeft;
    UBaseType_t uxLEDWifiBatStackLeft;
    UBaseType_t uxAWSIOTStackLeft;

    for( ;; ) 
    {
        /* Periodically poll every 5 seconds */
        vTaskDelay( pdMS_TO_TICKS( 5000 ) );

        if( Data_Aqu_Task_Handle != NULL ) 
        {
            /* Pass the specific task handle to check an external task */
            uxDataAquStackLeft = uxTaskGetStackHighWaterMark( Data_Aqu_Task_Handle );
            printf("[Monitor] Data Aquisition Task lowest historic free stack: %lu words\n", (unsigned long)uxDataAquStackLeft);
        }

        if( Model_Task_Handle != NULL ) 
        {
            /* Pass the specific task handle to check an external task */
            uxInferenceStackLeft = uxTaskGetStackHighWaterMark( Model_Task_Handle );
            printf("[Monitor] Inference Task lowest historic free stack: %lu words\n", (unsigned long)uxInferenceStackLeft);
        }

        if( LCD_BAT_WIFI_Task_Handle != NULL ) 
        {
            /* Pass the specific task handle to check an external task */
            uxLEDWifiBatStackLeft = uxTaskGetStackHighWaterMark( LCD_BAT_WIFI_Task_Handle );
            printf("[Monitor] LED/WiFi/Battery Task lowest historic free stack: %lu words\n", (unsigned long)uxLEDWifiBatStackLeft);
        }

         if( AWS_IOT_Task_Handle != NULL ) 
        {
            /* Pass the specific task handle to check an external task */
            uxAWSIOTStackLeft = uxTaskGetStackHighWaterMark( AWS_IOT_Task_Handle );
            printf("[Monitor] AWS IOT Task lowest historic free stack: %lu words\n", (unsigned long)uxAWSIOTStackLeft);
        }
    }
}


/*
Building the file:
In ~/Documents/Projects/Garden-IoT/freertos/rp2040-freertos:
> rm -rf build && mkdir build && cd build
> export PICO_SDK_PATH=/home/zoino/.pico-sdk/sdk/2.3.0  

> cmake -G Ninja .. \
  -DCMAKE_TOOLCHAIN_FILE=$PICO_SDK_PATH/cmake/preload/toolchains/pico_arm_cortex_m0plus_gcc.cmake \
  -DCMAKE_BUILD_TYPE=MinSizeRel \
  -DGFLAGS_INTTYPES_FORMAT=C99 \
  -DEXECUTORCH_BUILD_XNNPACK=OFF \
  -DEXECUTORCH_BUILD_CPUINFO=OFF \
  -DEXECUTORCH_BUILD_PTHREADPOOL=OFF \
  -DEXECUTORCH_BUILD_PORTABLE_KERNELS=ON \
  -DEXECUTORCH_BUILD_EXECUTOR_RUNNER=OFF \
  -DPython3_EXECUTABLE=(which python3)
> ninja


*/

int main()
{
    stdio_init_all();

     // Initialize chosen DHT22 pin
    gpio_init(DHT_PIN);

    // Initiialize lcd
    lcd_init();

    bh1750_init(I2C_PORT_BF1750);

    rs485_init();

    // CRITICAL: Initialize the wireless chip framework before using the LED
    if (cyw43_arch_init()) {
        printf("Wi-Fi architecture initialization failed!\n");
        return -1;
    }
  

    BaseType_t Data_Aqu_Task_status = xTaskCreate(
            data_aquisition_task,       // Function pointer
            "Data_aquisition_Task",    // Task name for debugging
            256,           // Stack depth
            NULL, // Parameter to pass (cast to void*)
            1,             // Priority
            &Data_Aqu_Task_Handle       // Task handle
    );

    BaseType_t Model_task_status = xTaskCreate(
        vInferenceTask,
        "ET_Inference",
        INFERENCE_TASK_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 2, // High priority to prevent scheduler preemption during inference
        &Model_Task_Handle
    );

    BaseType_t LCD_BAT_WIFI_task_status = xTaskCreate(
        lcd_wifi_bat_task,
        "ET_Inference",
        512,
        NULL,
        1, // High priority to prevent scheduler preemption during inference
        &LCD_BAT_WIFI_Task_Handle
    );

    BaseType_t Monitor_task_status = xTaskCreate( vMonitorTask, 
                 "MonitorTask", 
                 256, 
                 NULL, 
                 2,           // Higher priority to safely log data 
                 &xMonitorTaskHandle );
    
    BaseType_t AWS_IOT_task_status = xTaskCreate(vAwsIotTask, 
                "AWS_IoT_Task", 
                4096, // 4096 memory words for TLS context
                NULL, 
                tskIDLE_PRIORITY + 2, 
                &AWS_IOT_Task_Handle);

        // Add queue initialization before creating tasks
    xSensorQueue = xQueueCreate(5, sizeof(sens_package));
    xModelQueue  = xQueueCreate(5, sizeof(final_output_package));
    xLCDQueue  = xQueueCreate(5, sizeof(final_output_package));

    if (xSensorQueue == NULL || xModelQueue == NULL || xLCDQueue == NULL) {
        printf("[Fatal Error] Failed to create FreeRTOS queues.\n");
        while (true);
    }
    if (Model_task_status != pdPASS) {
        printf("[Fatal Error] Model Task creation failed.\n");
        while(true);
    }
    if (Data_Aqu_Task_status != pdPASS) {
        printf("[Fatal Error] Data Aquisition Task creation failed.\n");
        while(true);
    }
    if (LCD_BAT_WIFI_task_status != pdPASS) {
        printf("[Fatal Error] LCD/Wifi/Battery Task creation failed.\n");
        while(true);
    }
    if (Monitor_task_status != pdPASS) {
        printf("[Fatal Error] Monitor Task creation failed.\n");
        while(true);
    }
    if (AWS_IOT_task_status != pdPASS) {
        printf("[Fatal Error] AWS Task creation failed.\n");
        while(true);
    }

    vTaskCoreAffinitySet(Data_Aqu_Task_Handle, (1 << 0)); // Core 0
    vTaskCoreAffinitySet(LCD_BAT_WIFI_Task_Handle, (1 << 0)); // Core 0
    vTaskCoreAffinitySet(Model_Task_Handle, (1 << 1)); // Core 1
    vTaskCoreAffinitySet(AWS_IOT_Task_Handle, (1 << 1)); // Core 1

    vTaskStartScheduler();

    while(1){};
}