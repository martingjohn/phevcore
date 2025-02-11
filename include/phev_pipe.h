#ifndef _PHEV_PIPE_H_
#define _PHEV_PIPE_H_
#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif
#include <time.h>
#include <stdint.h>
#include <stdbool.h>
#include "msg_core.h"
#include "msg_pipe.h"
#include "phev_core.h"

#define PHEV_PIPE_MAX_EVENT_HANDLERS 10
#define PHEV_PIPE_MAX_UPDATE_CALLBACKS 10
#ifndef PHEV_CONNECT_WAIT_TIME
#define PHEV_CONNECT_WAIT_TIME (1000)
#endif

#ifndef PHEV_CONNECT_MAX_RETRIES
#define PHEV_CONNECT_MAX_RETRIES (5)
#endif

#define PHEV_PIPE_ECU_VERSION_SIZE 11
#define PHEV_PIPE_DATE_INFO_SIZE 6

#ifdef _WIN32
//  For Windows (32- and 64-bit)
#include <windows.h>
#define SLEEP(msecs) Sleep(msecs)
#elif defined(__unix) ||defined(__unix__)
//  For linux, OSX, and other unixes
#ifdef _POSIX_C_SOURCE
#undef _POSIX_C_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L // or greater
#include <time.h>
#define SLEEP(msecs)                      \
    do                                    \
    {                                     \
        struct timespec ts;               \
        ts.tv_sec = msecs / 1000;         \
        ts.tv_nsec = msecs % 1000 * 1000; \
        nanosleep(&ts, NULL);             \
    } while (0)
#elif __XTENSA__
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define SLEEP(msec) vTaskDelay(msec)
#else
#error "Unknown system"
#endif

enum
{
    PHEV_PIPE_GOT_VIN,
    PHEV_PIPE_CONNECTED,
    PHEV_PIPE_START_ACK,
    PHEV_PIPE_REGISTRATION,
    PHEV_PIPE_ECU_VERSION2,
    PHEV_PIPE_REMOTE_SECURTY_PRSNT_INFO,
    PHEV_PIPE_REG_DISP,
    PHEV_PIPE_MAX_REGISTRATIONS,
    PHEV_PIPE_REGISTRATION_COMPLETE,
    PHEV_PIPE_REG_UPDATE,
    PHEV_PIPE_REG_UPDATE_ACK,
    PHEV_PIPE_DATE_INFO,
    PHEV_PIPE_BB,
    PHEV_PIPE_PING_RESP,
    PHEV_PIPE_FILTERED_MESSAGE,
};
typedef struct phevPipeEvent_t
{
    int event;
    size_t length;
    void *data;
    void * ctx;
} phevPipeEvent_t;

typedef struct phevVinEvent_t
{
    char vin[18];
    uint8_t data;
    uint8_t registrations;
    void * ctx;
} phevVinEvent_t;

typedef struct phevError_t
{
    char * message;
} phevError_t;

typedef struct phev_pipe_ctx_t phev_pipe_ctx_t;
typedef int (* phevPipeEventHandler_t)(phev_pipe_ctx_t *ctx, phevPipeEvent_t *event);
typedef void (* phevErrorHandler_t)(phevError_t *error);
typedef void (* phev_pipe_updateRegisterCallback_t)(phev_pipe_ctx_t *ctx, uint8_t reg, void *customCtx);
typedef void (* phevRegistrationComplete_t)(phev_pipe_ctx_t *ctx);

typedef struct phev_pipe_updateRegisterCtx_t
{
    phev_pipe_updateRegisterCallback_t callbacks[PHEV_PIPE_MAX_UPDATE_CALLBACKS];
    bool used[PHEV_PIPE_MAX_UPDATE_CALLBACKS];
    uint8_t registers[PHEV_PIPE_MAX_UPDATE_CALLBACKS];
    uint8_t * values[PHEV_PIPE_MAX_UPDATE_CALLBACKS];
    size_t lengths[PHEV_PIPE_MAX_UPDATE_CALLBACKS];
    void * ctx[PHEV_PIPE_MAX_UPDATE_CALLBACKS];
    size_t numberOfCallbacks;
} phev_pipe_updateRegisterCtx_t;

typedef struct phev_pipe_ctx_t
{
    msg_pipe_ctx_t *pipe;
    phevPipeEventHandler_t eventHandler[PHEV_PIPE_MAX_EVENT_HANDLERS];
    int eventHandlers;
    phevErrorHandler_t errorHandler;
    time_t lastPingTime;
    uint8_t currentPing;
    uint8_t pingResponse;
    bool connected;
    phev_pipe_updateRegisterCtx_t *updateRegisterCallbacks;
    uint8_t currentXOR;
    uint8_t pingXOR;
    uint8_t commandXOR;
    bool encrypt;
    bool registerDevice;
    phevRegistrationComplete_t registrationCompleteCallback;
    void *ctx;
} phev_pipe_ctx_t;

typedef struct phev_pipe_settings_t
{
    messagingClient_t *in;
    messagingClient_t *out;
    msg_pipe_splitter_t inputSplitter;
    msg_pipe_aggregator_t inputAggregator;
    msg_pipe_splitter_t outputSplitter;
    msg_pipe_aggregator_t outputAggregator;
    msg_pipe_responder_t inputResponder;
    msg_pipe_responder_t outputResponder;
    msg_pipe_filter_t outputFilter;
    msg_pipe_filter_t inputFilter;
    msg_pipe_transformer_t inputInputTransformer;
    msg_pipe_transformer_t inputOutputTransformer;
    msg_pipe_transformer_t outputInputTransformer;
    msg_pipe_transformer_t outputOutputTransformer;
    msg_pipe_connectHook_t preConnectHook;
    phevErrorHandler_t errorHandler;
    bool registerDevice;
    phevRegistrationComplete_t registrationCompleteCallback;
    void *ctx;
} phev_pipe_settings_t;

void phev_pipe_loop(phev_pipe_ctx_t *);
phev_pipe_ctx_t *phev_pipe_createPipe(phev_pipe_settings_t);
void phev_pipe_waitForConnection(phev_pipe_ctx_t *ctx);
message_t *phev_pipe_outputChainInputTransformer(void *, message_t *);
message_t *phev_pipe_outputEventTransformer(void *, message_t *);
void phev_pipe_registerEventHandler(phev_pipe_ctx_t *, phevPipeEventHandler_t);
void phev_pipe_deregisterEventHandler(phev_pipe_ctx_t *, phevPipeEventHandler_t);
message_t *phev_pipe_commandResponder(void *, message_t *);
messageBundle_t *phev_pipe_outputSplitter(void *, message_t *);
void phev_pipe_ping(phev_pipe_ctx_t *);
void phev_pipe_resetPing(phev_pipe_ctx_t *);
void phev_pipe_start(phev_pipe_ctx_t *ctx, uint8_t *mac);
void phev_pipe_sendMac(phev_pipe_ctx_t *ctx, uint8_t *mac);
void phev_pipe_updateRegister(phev_pipe_ctx_t *, const uint8_t, const uint8_t);
void phev_pipe_updateComplexRegister(phev_pipe_ctx_t *, const uint8_t, const uint8_t *, size_t);
void phev_pipe_updateComplexRegisterWithCallback(phev_pipe_ctx_t *ctx, const uint8_t reg, const uint8_t * data, size_t length, phev_pipe_updateRegisterCallback_t callback, void * customCtx);
void phev_pipe_updateRegisterWithCallback(phev_pipe_ctx_t *ctx, const uint8_t reg, const uint8_t value, phev_pipe_updateRegisterCallback_t callback, void * customCtx);
phevPipeEvent_t *phev_pipe_createRegisterEvent(phev_pipe_ctx_t *phevCtx, phevMessage_t *phevMessage);
void phev_pipe_outboundPublish(phev_pipe_ctx_t * ctx, message_t * message);
void phev_pipe_pingOutboundPublish(phev_pipe_ctx_t * ctx, message_t * message);
void phev_pipe_commandOutboundPublish(phev_pipe_ctx_t * ctx, message_t * message);
void phev_pipe_sendRegister(phev_pipe_ctx_t * ctx);
void phev_pipe_destroyEvent(phevPipeEvent_t * event);
void phev_pipe_disconnectInput(phev_pipe_ctx_t *ctx);
void phev_pipe_disconnectOutput(phev_pipe_ctx_t *ctx);
void phev_pipe_sendEventToHandlers(phev_pipe_ctx_t *ctx, phevPipeEvent_t *event);

//void phev_pipe_sendCommand(phev_core_command_t);

#endif
