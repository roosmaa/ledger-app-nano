#include "coins.h"
#include "coins_dsl.h"

REGISTER_COINS(
    COIN(LIBN_COIN_TYPE_NANO, {
        /* coinName */ "Nano",
        /* coinBadge */ &C_nanos_badge_nano,
        /* bip32Prefix */ { HARDENED(44), HARDENED(165) },
        /* addressPrimaryPrefix */ "nano_",
        /* addressSecondaryPrefix */ "xrb_",
        /* addressDefaultPrefix */ LIBN_SECONDARY_PREFIX,
        /* defaultUnit */ "NANO",
        /* defaultUnitScale */ 30, // 1 Mnano = 10^30 raw
    })
)
