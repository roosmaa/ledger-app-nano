/*******************************************************************************
*   $NANO Wallet for Ledger Nano S & Blue
*   (c) 2016 Ledger
*   (c) 2018 Mart Roosmaa
*
*  Licensed under the Apache License, Version 2.0 (the "License");
*  you may not use this file except in compliance with the License.
*  You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
*  Unless required by applicable law or agreed to in writing, software
*  distributed under the License is distributed on an "AS IS" BASIS,
*  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*  See the License for the specific language governing permissions and
*  limitations under the License.
********************************************************************************/

#include "nano_ram_variables.h"
#include "os_io_seproxyhal.h"

// Bolos SDK variables
uint8_t G_io_seproxyhal_spi_buffer[IO_SEPROXYHAL_BUFFER_SIZE_B];
ux_state_t ux;

uint8_t nano_async_buffer_D[MAX_ADPU_OUTPUT_SIZE + 2 /* status word */];
nano_context_t nano_context_D;

#ifdef HAVE_U2F
u2f_service_t u2f_service_D;
uint8_t u2f_message_buffer_D[U2F_MAX_MESSAGE_SIZE];
bool u2f_activated_D;
#endif // HAVE_U2F