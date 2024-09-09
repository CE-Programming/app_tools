APP_TOOLS_DIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))

INIT_LOC = 0
LINKER_SCRIPT = linker_script_portable
OUTPUT_MAP = NO

APP_SRC_FILE = $(APP_TOOLS_DIR)/app.src

DEPS := $(APP_SRC_FILE) $(APP_TOOLS_DIR)/makefile $(DEPS)

EXTRA_LDFLAGS += \
	-i $(call QUOTE_ARG,provide __app_name = "$(APP_NAME)") \
	-i $(call QUOTE_ARG,provide __app_version = "$(APP_VERSION)") \
	-i $(call QUOTE_ARG,provide __app_desc = "$(DESCRIPTION)")

default: installer

include $(shell cedev-config --makefile)

TARGET8EK ?= $(NAME).8ek
APP_INST_NAME ?= APPINST

app: $(BINDIR)/$(TARGET8EK)
installer: $(BINDIR)/AppInstA.8xv

linker_script_portable: $(APP_TOOLS_DIR)/linker_script
	envsubst < $(APP_TOOLS_DIR)/linker_script > $@

$(BINDIR)/$(TARGET8EK): $(BINDIR)/$(TARGETBIN) $(APP_TOOLS_DIR)/make_8ek.py
	python3 $(APP_TOOLS_DIR)/make_8ek.py $(BINDIR)/$(TARGETBIN) $(BINDIR)/$(TARGET8EK) $(NAME)

$(BINDIR)/AppInstA.8xv: $(BINDIR)/$(TARGETBIN) $(APP_TOOLS_DIR)/installer.bin $(APP_TOOLS_DIR)/make_installer.py
	python3 $(APP_TOOLS_DIR)/make_installer.py $(BINDIR)/$(TARGETBIN) $(BINDIR) $(NAME) $(APP_INST_NAME)

.PHONY: default installer app
