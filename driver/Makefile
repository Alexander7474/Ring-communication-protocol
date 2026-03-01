BUILD_DIR  ?= ../build/driver
COMMON_DIR ?= ../build/common

SRCS   := $(wildcard *.c)
OBJS   := $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRCS))
DEPS   := $(OBJS:.o=.d)
TARGET := $(BUILD_DIR)/driver

.PHONY: all clean

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $@

$(TARGET): $(OBJS) $(COMMON_DIR)/libcommon.a
	$(CC) $(LDFLAGS) $^ -o $@

-include $(DEPS)

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

clean:
	rm -f $(OBJS) $(DEPS) $(TARGET)
