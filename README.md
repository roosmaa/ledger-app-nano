> [!warning]
> This repository is unmaintained. The repository for the current version of the app [available on Ledger devices] can be found at [LedgerHQ/app-nano](https://github.com/LedgerHQ/app-nano/).

# ledger-app-nano

$NANO wallet application for Ledger Nano S & Ledger Blue devices.

## For users

You can install the Nano app from the Ledger Manager. If it doesn't show up for you in Ledger Manager, make sure that your Ledger device firmware has been upgraded to the latest version. See also the Ledger own [guide on installing and using the app](https://support.ledgerwallet.com/hc/en-us/articles/360005459013-Install-and-use-Nano).

To interact with the Nano network, you need to use a wallet software that has integrated Ledger support. Currently the following wallets have integrated Ledger support:

- [Nault.cc](https://nault.cc/) ([user guide](https://docs.nault.cc/2020/08/04/ledger-guide.html))

_If a wallet is missing from above, please [create an issue](https://github.com/roosmaa/blue-app-nano/issues/new) and it will be added to this list._

## For wallet developers

If you wish to integrate your $NANO web wallet with Ledger, then you can use the [hw-app-nano](https://github.com/roosmaa/hw-app-nano/) JavaScript library that works in tandem with [ledgerjs](https://github.com/LedgerHQ/ledgerjs) library.

For desktop wallet apps, there is no integration library at the time. But you can still interact with the device using the binary protocol detailed in [the ADPU documentation](https://github.com/roosmaa/blue-app-nano/blob/master/doc/nano.md).
