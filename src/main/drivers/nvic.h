#pragma once

// can't use 0
#define NVIC_PRIO_MAX                      NVIC_BUILD_PRIORITY(0, 1)

// utility macros to join/split priority
#define NVIC_PRIORITY_GROUPING NVIC_PRIORITYGROUP_2
#define NVIC_BUILD_PRIORITY(base,sub) (((((base)<<(4-(7-(NVIC_PRIORITY_GROUPING))))|((sub)&(0x0f>>(7-(NVIC_PRIORITY_GROUPING)))))<<4)&0xf0)
#define NVIC_PRIORITY_BASE(prio) (((prio)>>(4-(7-(NVIC_PRIORITY_GROUPING))))>>4)
#define NVIC_PRIORITY_SUB(prio) (((prio)&(0x0f>>(7-(NVIC_PRIORITY_GROUPING))))>>4)
