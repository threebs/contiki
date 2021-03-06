/*
 * Copyright (c) 2015, 3B Scientific GmbH.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * This file is part of the Contiki operating system.
 *
 */

/**
 * \file
 *         Headers for clock architecture implementation of TI CC32xx.
 * \author
 *         Björn Rennfanz <bjoern.rennfanz@3bscientific.com>
 */

#ifndef CLOCK_ARCH_H_
#define CLOCK_ARCH_H_

#include "contiki-conf.h"
#include "rtimer-arch.h"

#define CLOCK_ARCH_CPU					80000000
#define RTIMER_TO_CLOCK_SECOND			(RTIMER_ARCH_SECOND / CLOCK_CONF_SECOND)

#if defined(USE_FREERTOS) || defined(USE_TIRTOS)
#define CLOCK_ARCH_TICK_MS				(1000 / RTIMER_ARCH_SECOND)
#else
#define CLOCK_ARCH_PRELOAD				(CLOCK_ARCH_CPU / RTIMER_ARCH_SECOND)
#endif

#if defined(USE_FREERTOS) || defined(USE_TIRTOS)
#define CLOCK_ARCH_TICKTASK_PRIORITY	8
#define CLOCK_ARCH_TICKTASK_STACKSIZE	256
#endif

#define USEC_TO_LOOP(x)					((CLOCK_ARCH_CPU/5000000)*x)

/**
 * Start the clock, by creating timer service
 */
void clock_arch_init(void);

#if defined(USE_FREERTOS) || defined(USE_TIRTOS)
/**
 * Task for time service
 */
void clock_arch_tick_task(void *pv_parameters);
#else
/**
 * Systick isr for time service
 */
void clock_arch_isr(void);
#endif
/**
 * Update the software clock ticks
 */
void clock_arch_update(void);

/**
 * Return tick counter, default is ticks since startup.
 */
inline clock_time_t clock_arch_get_tick_count(void);

/**
 * Set tick counter.
 */
inline void clock_arch_set_tick_count(clock_time_t t);

#endif /* RTIMER_ARCH_H_ */
