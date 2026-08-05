#ifndef _LWIPOPTS_H
#define _LWIPOPTS_H

// Core lock-free and thread-safe options for FreeRTOS
#define SYS_LIGHTWEIGHT_PROT            1
#define NO_SYS                          0

// Prevent lwIP from redefining 'struct timeval'
// This stops the duplicate declaration crash against your GCC compiler toolchain
#define LWIP_TIMEVAL_PRIVATE            0

// Memory setups
#define MEM_ALIGNMENT                   4
#define MEM_SIZE                        (16 * 1024)
#define MEMP_NUM_PBUF                   10
#define MEMP_NUM_UDP_PCB                6
#define MEMP_NUM_TCP_PCB                10
#define MEMP_NUM_TCP_PCB_LISTEN         5
#define MEMP_NUM_TCP_SEG                32
#define MEMP_NUM_SYS_TIMEOUT            10

// Increase the pool size and buffer capacity so the window fits comfortably
#define PBUF_POOL_SIZE                  24       // Increased from 12
#define PBUF_POOL_BUFSIZE               512      // Increased from 256

// TCP Feature Flags
#define LWIP_TCP                        1
#define TCP_TTL                         255
#define TCP_QUEUE_OOO                   0
#define TCP_MSS                         (1500 - 40)
#define TCP_SND_BUF                     (4 * TCP_MSS)
#define TCP_SND_QUEUELEN                (2 * TCP_SND_BUF/TCP_MSS)
#define TCP_WND                         (4 * TCP_MSS)

// UDP and DNS Feature Flags
#define LWIP_UDP                        1
#define LWIP_DNS                        1
#define LWIP_DHCP                       1

// Socket and Thread Options
#define LWIP_SOCKET                     1
#define LWIP_NETCONN                    1
#define TCPIP_MBOX_SIZE                 8
#define DEFAULT_RAW_RECVMBOX_SIZE       8
#define DEFAULT_UDP_RECVMBOX_SIZE       8
#define DEFAULT_TCP_RECVMBOX_SIZE       8
#define DEFAULT_ACCEPTMBOX_SIZE         8

// Enable SNTP client support
#define LWIP_SNTP                       1

#endif /* _LWIPOPTS_H */
