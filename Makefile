ifeq ($(APP_PARAM), )
    APP_PARAM := ../Makefile.param
    include $(APP_PARAM)
endif

export LC_ALL := C
SHELL := /bin/bash

CURRENT_DIR := $(shell pwd)

PKG_NAME := read_mlx90614
PKG_BIN  ?= out

READ_mlx90614_CFLAGS  := $(RK_APP_OPTS) $(RK_APP_CROSS_CFLAGS)
READ_mlx90614_LDFLAGS := $(RK_APP_OPTS) $(RK_APP_LDFLAGS)

PKG_TARGET := read_mlx90614-build

all: $(PKG_TARGET)
	@echo "build $(PKG_NAME) done"

read_mlx90614-build:
	@rm -rf $(PKG_BIN); \
	mkdir -p $(PKG_BIN)/bin
	# Compile I2C C program
	$(RK_APP_CROSS)-gcc $(READ_mlx90614_CFLAGS) read_mlx90614.c -o $(PKG_BIN)/bin/read_mlx90614 $(READ_mlx90614_LDFLAGS)
	
	# Compile IIO C program
	$(RK_APP_CROSS)-gcc $(READ_mlx90614_CFLAGS) read_mlx90614_iio.c -o $(PKG_BIN)/bin/read_mlx90614_iio $(READ_mlx90614_LDFLAGS)
	
	# Copy shell scripts and make them executable
	cp read_mlx90614.sh $(PKG_BIN)/bin/
	chmod +x $(PKG_BIN)/bin/read_mlx90614.sh
	cp read_mlx90614_iio.sh $(PKG_BIN)/bin/
	chmod +x $(PKG_BIN)/bin/read_mlx90614_iio.sh
	
	# Copy built files to app output directory
	$(call MAROC_COPY_PKG_TO_APP_OUTPUT, $(RK_APP_OUTPUT), $(PKG_BIN))

clean:
	@rm -rf $(PKG_BIN)

distclean: clean

info:
	@echo "This is $(PKG_NAME) for $(RK_APP_CHIP) with $(RK_APP_CROSS)
