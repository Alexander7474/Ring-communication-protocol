CC      := gcc
CFLAGS  := -Wall -std=gnu11
LDFLAGS :=

RED=\033[0;31m
GREEN=\033[0;32m
CYAN=\033[0;36m
ORANGE=\033[0;33m
NC=\033[0m # No Color

ifeq ($(DEBUG),1)
    CFLAGS += -g3 -O0 -DDEBUG
else
    CFLAGS += -O2 -DNDEBUG
endif

export CC CFLAGS LDFLAGS

.PHONY: all clean common driver comm

all: common driver comm
	@echo "$(GREEN)========================================"
	@echo " Build complete"
	@echo "   driver : $(CYAN)build/driver$(GREEN)"
	@echo "   comm   : $(CYAN)build/comm$(GREEN)"
	@echo "========================================$(NC)"

# explicit ordering — driver and comm must wait for common
common:
	@echo "$(GREEN)----------------------------------------"
	@echo " Building static library : common"
	@echo "----------------------------------------$(NC)"
	$(MAKE) -C common BUILD_DIR=$(CURDIR)/build/common

driver: common
	@echo "$(GREEN)----------------------------------------"
	@echo " Building binary : driver"
	@echo "----------------------------------------$(NC)"
	$(MAKE) -C driver BUILD_DIR=$(CURDIR)/build/driver COMMON_DIR=$(CURDIR)/build/common

comm: common
	@echo "$(GREEN)----------------------------------------"
	@echo " Building binary : comm"
	@echo "----------------------------------------$(NC)"
	$(MAKE) -C comm BUILD_DIR=$(CURDIR)/build/comm COMMON_DIR=$(CURDIR)/build/common

clean:
	@echo "$(ORANGE)----------------------------------------"
	@echo " Cleaning all build artifacts"
	@echo "----------------------------------------$(NC)"
	$(MAKE) -C common clean BUILD_DIR=$(CURDIR)/build/common
	$(MAKE) -C driver clean BUILD_DIR=$(CURDIR)/build/driver
	$(MAKE) -C comm   clean BUILD_DIR=$(CURDIR)/build/comm
	rm -rf build/
	@echo "$(ORANGE)  done.$(NC)"
