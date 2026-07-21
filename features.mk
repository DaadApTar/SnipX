SENDER ?= 1
COLORS ?= 1
ZSTD   ?= 1
LZ4    ?= 1

ifeq ($(SENDER), 1)
SENDER_TARGET := $(BUILD_DIR)/$(SENDER_TARGET_NAME)
CFLAGS += -DFEATURE_SENDER
endif

ifeq ($(COLORS), 1)
CFLAGS += -DFEATURE_COLORS
endif

ifeq ($(ZSTD), 1)
CFLAGS += -DFEATURE_ZSTD
PKGS   += libzstd
endif

ifeq ($(LZ4), 1)
CFLAGS += -DFEATURE_LZ4
PKGS   += liblz4
endif
