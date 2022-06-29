# todo: determine this automatically
APP_TOOLS_DIR = app_tools

INIT_LOC = 0
LINKER_SCRIPT = app_tools/linker_script
OUTPUT_MAP = NO

DEPS := $(APP_TOOLS_DIR)/app.src $(APP_TOOLS_DIR)/makefile $(DEPS)
TEMP := $(EXTRA_LDFLAGS)

EXTRA_LDFLAGS = \
	-i $(call QUOTE_ARG,provide __app_name = "$(APP_NAME)") \
	-i $(call QUOTE_ARG,provide __app_version = "$(APP_VERSION)") \
	-i $(call QUOTE_ARG,provide __app_desc = "$(DESCRIPTION)") \
	$(TEMP)

app: _app

include $(shell cedev-config --makefile)

TARGET8EK ?= $(NAME).8ek

_app: $(BINDIR)/$(TARGET8EK)

$(BINDIR)/$(TARGET8EK): $(BINDIR)/$(TARGETBIN) $(APP_TOOLS_DIR)/make_app.py
	python3 $(APP_TOOLS_DIR)/make_app.py $(BINDIR)/$(TARGETBIN) $(BINDIR)/$(TARGET8EK) $(NAME)

.PHONY: _app app