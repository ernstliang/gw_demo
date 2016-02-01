TARGETS := GatewayDemo
OBJS	:= $(patsubst %.c, %.o, $(wildcard *.c))
CROSS	:= 

PLATFORM:=

CFLAGS:= -m32 -g -Wall -D_USE_GW_SDK_
CXXFLAGS:=

USR_LIB	:=

# export DIR_BUILD_ROOT=$(shell pwd)

# 读取synology.conf中的配置
# PLTF_CONFIG:=$(shell pwd)/$(PLATFORM).conf
# -include $(PLTF_CONFIG)


#需要包含的头文件搜索路径
INCLUDE += -I../txdevicesdk/include   #txdevicesdk headers
INCLUDE += -I../include               #libgwsdk.so headers
INCLUDE += -I./

#需要引用的库
#LIB     += -L../txdevicesdk/ -ltxdevicesdk   
LIB		+= -L../GatewaySDK -lgwsdk
LIB     += -lpthread  -lrt -lm -ldl

#需要的标志位
CC	:= $(CROSS)gcc
#CXX :=$(CROSS)g++
STRIP:=$(CROSS)strip

#增加根据d文件自动推导编译的标志
#override CXXFLAGS += $(INCLUDE)
CFLAGS += $(INCLUDE)

#deps
DEPS    += $(patsubst %.o, %.d, $(OBJS))

.PHONY: all
all: $(DEPS) $(OBJS)
	$(CC) $(OBJS) $(LIB) -o $(TARGETS)
	$(STRIP) $(TARGETS)

-include $(DEPS)

%.d: %.c
	$(CC) -MM $(CFLAGS) -c $< -o $@

.PHONY:clean
clean:
	rm -rf $(TARGETS) $(OBJS) $(DEPS)

