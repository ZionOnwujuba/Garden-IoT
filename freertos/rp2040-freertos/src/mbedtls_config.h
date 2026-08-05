#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

/* Forward to default mbedTLS configuration provided by the Pico SDK */
#include "mbedtls/mbedtls_config.h"

/* Alternate timing and hardware entropy setup */
#define MBEDTLS_NO_PLATFORM_ENTROPY
#define MBEDTLS_ENTROPY_HARDWARE_ALT
#define MBEDTLS_PLATFORM_MS_TIME_ALT

/* Required Cryptographic and Protocol Modules */
#define MBEDTLS_ENTROPY_C
#define MBEDTLS_CTR_DRBG_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_AES_C

#define MBEDTLS_PSA_CRYPTO_CONFIG
#define MBEDTLS_PSA_CRYPTO_EXTERNAL_RNG

/* MUST be disabled on bare-metal / RP2040 (No POSIX sys/socket.h) */
#undef MBEDTLS_NET_C

/* Disable OS-specific modules */
#undef MBEDTLS_TIMING_C
#undef MBEDTLS_FS_IO
#undef MBEDTLS_PSA_ITS_FILE_C
#undef MBEDTLS_PSA_CRYPTO_STORAGE_C
#define MBEDTLS_PSA_CRYPTO_C

#endif /* MBEDTLS_CONFIG_H */