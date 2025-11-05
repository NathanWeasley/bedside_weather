#include "tasks/comm_task.h"
#include "usart.h"
#include "utils/cbuffer.h"

static circular_buffer_t g_cbuf = { 0 };

void comm_task_init(task_param_t * param)
{
    MX_USART1_UART_StartReceive();
}

