#*******************************************************************************
#   Ledger App
#   (c) 2017 Ledger
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#*******************************************************************************

ifeq ($(BOLOS_SDK),)
$(error Environment variable BOLOS_SDK is not set)
endif
ifeq (customCA.key,$(wildcard customCA.key))
    SCP_PRIVKEY=`cat customCA.key`
endif
include $(BOLOS_SDK)/Makefile.defines

# Default to shared app
ifeq ($(APP_TYPE),)
APP_TYPE=shared
endif

# Default to library app
ifndef COIN
COIN=nano
endif

APP_LOAD_PARAMS = --curve ed25519 $(COMMON_LOAD_PARAMS)
ALL_PATH_PARAMS =

# Nano coin config
NANO_APP_NAME = "Nano"
NANO_PATH_PARAM = --path "44'/165'"
NANO_COIN_TYPE = LIBN_COIN_TYPE_NANO
ALL_PATH_PARAMS += $(NANO_PATH_PARAM)

# Banano coin config
BANANO_APP_NAME = "Banano"
BANANO_PATH_PARAM = --path "44'/198'"
BANANO_COIN_TYPE = LIBN_COIN_TYPE_BANANO
ALL_PATH_PARAMS += $(BANANO_PATH_PARAM)

# NOS coin config
NOS_APP_NAME = "NOS"
NOS_PATH_PARAM = --path "44'/229'"
NOS_COIN_TYPE = LIBN_COIN_TYPE_NOS
ALL_PATH_PARAMS += $(NOS_PATH_PARAM)

ifeq ($(APP_TYPE), standalone)
LIB_LOAD_FLAGS = --appFlags 0x50
APP_LOAD_FLAGS = --appFlags 0x50
DEFINES += IS_STANDALONE_APP

else ifeq ($(APP_TYPE), shared)
LIB_LOAD_FLAGS = --appFlags 0x850
APP_LOAD_FLAGS = --appFlags 0x50 --dep Nano
DEFINES += SHARED_LIBRARY_NAME=\"$(NANO_APP_NAME)\"
DEFINES += HAVE_COIN_NANO
DEFINES += HAVE_COIN_BANANO
DEFINES += HAVE_COIN_NOS

else
ifneq ($(MAKECMDGOALS),listvariants)
$(error Unsupported APP_TYPE - use standalone, shared)
endif
endif

ifeq ($(COIN),nano)
APPNAME = $(NANO_APP_NAME)
ifeq ($(APP_TYPE), shared)
APP_LOAD_PARAMS += $(LIB_LOAD_FLAGS) $(ALL_PATH_PARAMS)
DEFINES += IS_SHARED_LIBRARY
else
APP_LOAD_PARAMS += $(LIB_LOAD_FLAGS) $(NANO_PATH_PARAM)
endif
DEFINES += HAVE_COIN_NANO
DEFINES += DEFAULT_COIN_TYPE=$(NANO_COIN_TYPE)

else ifeq ($(COIN),banano)
APPNAME = $(BANANO_APP_NAME)
APP_LOAD_PARAMS += $(APP_LOAD_FLAGS) $(BANANO_PATH_PARAM)
DEFINES += HAVE_COIN_BANANO
DEFINES += DEFAULT_COIN_TYPE=$(BANANO_COIN_TYPE)

else ifeq ($(COIN),nos)
APPNAME = $(NOS_APP_NAME)
APP_LOAD_PARAMS += $(APP_LOAD_FLAGS) $(NOS_PATH_PARAM)
DEFINES += HAVE_COIN_NOS
DEFINES += DEFAULT_COIN_TYPE=$(NOS_COIN_TYPE)

else ifeq ($(filter clean listvariants,$(MAKECMDGOALS)),)
$(error Unsupported COIN - use nano, banano, nos)
endif

APPVERSION_M=1
APPVERSION_N=2
APPVERSION_P=0
APPVERSION=$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)

MAX_ADPU_INPUT_SIZE=217
MAX_ADPU_OUTPUT_SIZE=98

ifeq ($(TARGET_NAME),TARGET_BLUE)
ICONNAME=blue_icon_$(COIN).gif
else
ICONNAME=nanos_icon_$(COIN).gif
endif

################
# Default rule #
################

all: default

############
# Platform #
############

DEFINES   += OS_IO_SEPROXYHAL IO_SEPROXYHAL_BUFFER_SIZE_B=300
DEFINES   += HAVE_BAGL HAVE_SPRINTF
DEFINES   += HAVE_IO_USB HAVE_L4_USBLIB IO_USB_MAX_ENDPOINTS=6 IO_HID_EP_LENGTH=64 HAVE_USB_APDU
DEFINES   += APP_MAJOR_VERSION=$(APPVERSION_M) APP_MINOR_VERSION=$(APPVERSION_N) APP_PATCH_VERSION=$(APPVERSION_P)
DEFINES   += MAX_ADPU_OUTPUT_SIZE=$(MAX_ADPU_OUTPUT_SIZE)

# U2F
DEFINES   += HAVE_IO_U2F
DEFINES   += U2F_PROXY_MAGIC=\"mRB\"
DEFINES   += USB_SEGMENT_SIZE=64
DEFINES   += BLE_SEGMENT_SIZE=32 #max MTU, min 20
DEFINES   += U2F_REQUEST_TIMEOUT=10000 # 10 seconds
DEFINES   += UNUSED\(x\)=\(void\)x
DEFINES   += APPVERSION=\"$(APPVERSION)\"

# Ed25519 (donna)
DEFINES   += ED25519_CUSTOMHASH
DEFINES   += ED25519_CUSTOMRANDOM

# WebUSB
WEBUSB_URL = www.ledgerwallet.com
DEFINES   += HAVE_WEBUSB WEBUSB_URL_SIZE_B=$(shell echo -n $(WEBUSB_URL) | wc -c) WEBUSB_URL=$(shell echo -n $(WEBUSB_URL) | sed -e "s/./\\\'\0\\\',/g")

# Enabling debug PRINTF
DEBUG = 0
ifneq ($(DEBUG),0)

        ifeq ($(TARGET_NAME),TARGET_NANOX)
                DEFINES   += HAVE_PRINTF PRINTF=mcu_usb_printf
        else
                DEFINES   += HAVE_PRINTF PRINTF=screen_printf
        endif
else
        DEFINES   += PRINTF\(...\)=
endif

##############
# Compiler #
##############
ifneq ($(BOLOS_ENV),)
$(info BOLOS_ENV=$(BOLOS_ENV))
CLANGPATH := $(BOLOS_ENV)/clang-arm-fropi/bin/
GCCPATH := $(BOLOS_ENV)/gcc-arm-none-eabi-5_3-2016q1/bin/
else
$(info BOLOS_ENV is not set: falling back to CLANGPATH and GCCPATH)
endif
ifeq ($(CLANGPATH),)
$(info CLANGPATH is not set: clang will be used from PATH)
endif
ifeq ($(GCCPATH),)
$(info GCCPATH is not set: arm-none-eabi-* will be used from PATH)
endif

CC       := $(CLANGPATH)clang

#CFLAGS   += -O0
CFLAGS   += -O3 -Os -Wno-typedef-redefinition

AS     := $(GCCPATH)arm-none-eabi-gcc

LD       := $(GCCPATH)arm-none-eabi-gcc
LDFLAGS  += -O3 -Os
LDLIBS   += -lm -lgcc -lc

# import rules to compile glyphs(/pone)
include $(BOLOS_SDK)/Makefile.glyphs

### variables processed by the common makefile.rules of the SDK to grab source files and include dirs
APP_SOURCE_PATH  += src vendor/ed25519-donna vendor/blake2
SDK_SOURCE_PATH  += lib_stusb
SDK_SOURCE_PATH  += lib_stusb_impl
SDK_SOURCE_PATH  += lib_u2f

load: all
	python -m ledgerblue.loadApp $(APP_LOAD_PARAMS)

delete:
	python -m ledgerblue.deleteApp $(COMMON_DELETE_PARAMS)

# import generic rules from the sdk
include $(BOLOS_SDK)/Makefile.rules

#add dependency on custom makefile filename
dep/%.d: %.c Makefile

listvariants:
	@echo VARIANTS COIN nano banano nos
