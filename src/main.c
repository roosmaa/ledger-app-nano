/*******************************************************************************
*   $NANO Wallet for Ledger Nano S & Blue
*   (c) 2018 Mart Roosmaa
*   (c) 2016 Ledger
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

#include "os_io_seproxyhal.h"
#include "libn_internal.h"
#include "coins.h"

#if defined(IS_SHARED_LIBRARY) || defined(IS_STANDALONE_APP)

__attribute__((section(".boot"))) int main(int arg0) {
    // exit critical section
    __asm volatile("cpsie i");

#ifdef IS_SHARED_LIBRARY
    const uint32_t *libcall_args = (uint32_t *)arg0;

    if (libcall_args) {
        if (libcall_args[0] != 0x100) {
            os_lib_throw(INVALID_PARAMETER);
        }
        // grab the coin type from the first parameter
        init_coin_config((libn_coin_type_t)libcall_args[1]);
    } else {
        init_coin_config(DEFAULT_COIN_TYPE);
    }
#else
    init_coin_config(DEFAULT_COIN_TYPE);
#endif

    // ensure exception will work as planned
    os_boot();

    for (;;) {
        BEGIN_TRY {
            TRY {
                app_init();
                app_main();
            }
            CATCH(EXCEPTION_IO_RESET) {
                // reset IO and UX
                continue;
            }
            CATCH_ALL {
                break;
            }
            FINALLY {
            }
        }
        END_TRY;
    }

    app_exit();
    return 0;
}

#else // IS_SHARED_LIBRARY || IS_STANDALONE_APP

__attribute__((section(".boot"))) int main(void) {
    // in RAM allocation (on stack), to allow simple simple traversal into the
    // main Nano app (separate NVRAM zone)
    uint32_t libcall_params[3];
    BEGIN_TRY {
        TRY {
            // ensure syscall will accept us
            check_api_level(CX_COMPAT_APILEVEL);
            // delegate to Nano app/lib
            libcall_params[0] = SHARED_LIBRARY_NAME;
            libcall_params[1] = 0x100; // use the Init call, as we won't exit
            libcall_params[2] = DEFAULT_COIN_TYPE;
            os_lib_call(&libcall_params);
        }
        FINALLY {
            app_exit();
        }
    }
    END_TRY;
    return 0;
}

#endif // IS_SHARED_LIBRARY || IS_STANDALONE_APP
