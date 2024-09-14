# ----------------------------
# Makefile Options
# ----------------------------

COMPRESSED ?= NO
ARCHIVED ?= NO
LTO ?= NO

EXTRA_LDFLAGS += \
	-i $(call QUOTE_ARG,provide __app_bin = "$(APP_BIN)") \
	-i $(call QUOTE_ARG,provide __appvar_prefix = "$(VAR_PREFIX)")

DEPS += $(APP_BIN)

CFLAGS ?= -Wall -Wextra -Oz
CXXFLAGS ?= -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)
