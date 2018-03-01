# $NANO application : Common Technical Specifications

## About

This specification describes the APDU messages interface to communicate with the $NANO application.

## Wallet usage APDUs

### Get address

#### Description

This command returns the public key and the encoded address for the given BIP 32 path.

#### Coding

**Command**

| *CLA* | *INS*  | *P1*                             | *P2* | *Lc* | *Le* |
|-------|--------|----------------------------------|------|------|------|
|   A1  |   02   |  00 : do not display the address |      |      |      |
|       |        |  01 : display the address        |      |      |      |

**Input data**

| *Description*                                     | *Length*  |
|---------------------------------------------------|-----------|
| Number of BIP 32 derivations to perform (max 10)  | 1         |
| First derivation index (big endian)               | 4         |
| ...                                               | 4         |
| Last derivation index (big endian)                | 4         |

**Output data**

| *Description*                                     | *Length*  |
|---------------------------------------------------|-----------|
| Public key                                        | 32        |
| Account address length                            | 1         |
| Account address                                   | var       |


### Validate block

#### Description

This command validates the signature of an universal block and caches the block data in-memory so that the next call to sign block command could determine the changes in account state.

#### Coding

**Command**

| *CLA* | *INS*  | *P1* | *P2* | *Lc* | *Le* |
|-------|--------|------|------|------|------|
|   A1  |   03   |  00  |  00  |      |      |


**Input data**

| *Description*                                      | *Length*  |
|----------------------------------------------------|-----------|
| Number of BIP 32 derivations to perform (max 10)   | 1         |
| First derivation index (big endian)                | 4         |
| ...                                                | 4         |
| Last derivation index (big endian)                 | 4         |
| Parent block hash                                  | 32        |
| Link                                               | 32        |
| Representative                                     | 32        |
| Balance                                            | 16        |
| Signature                                          | 64        |


**Output data**

*None*


### Sign block

#### Description

This command returns the signature for the provided universal block data. For non-null parent blocks the validate block command needs to be called before the this command.

#### Coding

**Command**

| *CLA* | *INS*  | *P1* | *P2*        | *Lc* | *Le* |
|-------|--------|------|-------------|------|------|
|   A1  |   04   |  00  | *see below* |      |      |

The ***P2*** value can compose of the following bitwise flags:

- `0x01` - Use *xrb_* prefix for recipient address (instead of the default *nano_* one) when confirming change with user
- `0x02` - Use *xrb_* prefix for representative address (instead of the default *nano_* one) when confirming change with user


**Input data**

| *Description*                                      | *Length*  |
|----------------------------------------------------|-----------|
| Number of BIP 32 derivations to perform (max 10)   | 1         |
| First derivation index (big endian)                | 4         |
| ...                                                | 4         |
| Last derivation index (big endian)                 | 4         |
| Parent block hash                                  | 32        |
| Target                                             | 32        |
| Representative                                     | 32        |
| Balance                                            | 16        |


**Output data**

| *Description*                                      | *Length*  |
|----------------------------------------------------|-----------|
| Block hash                                         | 32        |
| Signature                                          | 64        |


## Test and utility APDUs

### Get app configuration

#### Description

This command returns the application configuration.

#### Coding

**Command**

| *CLA* | *INS*  | *P1*                 | *P2* | *Lc* | *Le* |
|-------|--------|----------------------|------|------|------|
|   A1  |   01   |  00                  |  00  |  00  |      |

**Input data **

*None*

**Output data**

| *Description*                                      | *Length*  |
|----------------------------------------------------|-----------|
| Major app version                                  | 1         |
| Minor app version                                  | 1         |
| Patch app version                                  | 1         |


## Transport protocol

### General transport description

Ledger APDUs requests and responses are encapsulated using a flexible protocol allowing to fragment large payloads over different underlying transport mechanisms. 

The common transport header is defined as follows:

| *Description*                                                                     | *Length* |
|-----------------------------------------------------------------------------------|----------|
| Communication channel ID (big endian)                                             | 2        |
| Command tag                                                                       | 1        |
| Packet sequence index (big endian)                                                | 2        |
| Payload                                                                           | var      |

The Communication channel ID allows commands multiplexing over the same physical link. It is not used for the time being, and should be set to 0101 to avoid compatibility issues with implementations ignoring a leading 00 byte.

The Command tag describes the message content. Use TAG_APDU (0x05) for standard APDU payloads, or TAG_PING (0x02) for a simple link test.

The Packet sequence index describes the current sequence for fragmented payloads. The first fragment index is 0x00.

### APDU Command payload encoding

APDU Command payloads are encoded as follows:

| *Description*                                                                     | *Length* |
|-----------------------------------------------------------------------------------|----------|
| APDU length (big endian)                                                          | 2        |
| APDU CLA                                                                          | 1        |
| APDU INS                                                                          | 1        |
| APDU P1                                                                           | 1        |
| APDU P2                                                                           | 1        |
| APDU length                                                                       | 1        |
| Optional APDU data                                                                | var      |

APDU payload is encoded according to the APDU case 

| Case Number  | *Lc* | *Le* | Case description                                          |
|--------------|------|------|-----------------------------------------------------------|
|   1          |  0   |  0   | No data in either direction - L is set to 00              |
|   2          |  0   |  !0  | Input Data present, no Output Data - L is set to Lc       |
|   3          |  !0  |  0   | Output Data present, no Input Data - L is set to Le       |
|   4          |  !0  |  !0  | Both Input and Output Data are present - L is set to Lc   |

### APDU Response payload encoding

APDU Response payloads are encoded as follows:

| *Description*                                                                     | *Length* |
|-----------------------------------------------------------------------------------|----------|
| APDU response length (big endian)                                                 | 2        |
| APDU response data and Status Word                                                | var      |

### USB mapping

Messages are exchanged with the dongle over HID endpoints over interrupt transfers, with each chunk being 64 bytes long. The HID Report ID is ignored.

## Status words 

The following standard Status Words are returned for all APDUs - some specific Status Words can be used for specific commands and are mentioned in the command description.

**Status words**

|   *SW*   | *Description*                                                                 |
|----------|-------------------------------------------------------------------------------|
|   6700   | Incorrect length                                                              |
|   6982   | Security status not satisfied (dongle is locked or busy with another request) |
|   6985   | User declined the request                                                     |
|   6A80   | Invalid input data                                                            |
|   6A81   | Parent block data cache-miss (exec validate block command)                    |
|   6B00   | Incorrect parameter P1 or P2                                                  |
|   6Fxx   | Technical problem (Internal error, please report)                             |
|   9000   | Normal ending of the command                                                  |
