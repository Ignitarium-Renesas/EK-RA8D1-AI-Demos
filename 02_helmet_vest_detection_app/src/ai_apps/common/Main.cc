/*
 * SPDX-FileCopyrightText: Copyright 2021-2022 Arm Limited and/or its affiliates <open-source-office@arm.com>
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/****************************************************************************\
 *               Main application file for ARM NPU on MPS3 board             *
\****************************************************************************/

#include "log_macros.h"
#include "TensorFlowLiteMicro.hpp"  /* our inference logic api */
#include "BufAttributes.hpp"
#include "hal_data.h"
#include "r_typedefs.h"


uint8_t tensorArena[0x00A8000];

extern void main_loop_obj();
extern bool_t do_detection;

#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6010050)
__ASM(" .global __ARM_use_no_argv\n");
#endif

/* Print application information. */
static void print_application_intro()
{
    info("Copyright 2021-2022 Arm Limited and/or its affiliates <open-source-office@arm.com>");
}
extern "C" int run_apps();


int run_apps()
{
    main_loop_obj();
    return 0;
}
