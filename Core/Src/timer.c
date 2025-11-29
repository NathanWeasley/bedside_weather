#include "main.h"
#include "config.h"

void MX_TIM6_Init()
{
    LL_TIM_InitTypeDef TIM_InitStruct = {0};

    LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM6);

    NVIC_SetPriority(TIM6_IRQn, 2);
    NVIC_EnableIRQ(TIM6_IRQn);

    TIM_InitStruct.Prescaler = 64 - 1;      //< 1MHz clock (1us)
    TIM_InitStruct.CounterMode = LL_TIM_COUNTERDIRECTION_UP;
    TIM_InitStruct.Autoreload = TASK_SCHEDULE_PERIOD_US - 1;
    TIM_InitStruct.ClockDivision = LL_TIM_CLOCKDIVISION_DIV1;
    LL_TIM_Init(TIM6, &TIM_InitStruct);
    LL_TIM_EnableARRPreload(TIM6);
    LL_TIM_SetClockSource(TIM6, LL_TIM_CLOCKSOURCE_INTERNAL);
    LL_TIM_SetTriggerOutput(TIM6, LL_TIM_TRGO_RESET);
    LL_TIM_DisableMasterSlaveMode(TIM6);
    LL_TIM_EnableIT_UPDATE(TIM6);

    LL_TIM_ClearFlag_UPDATE(TIM6);
    LL_TIM_EnableCounter(TIM6);
}
