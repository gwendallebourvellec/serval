LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

#
# Scaffold
#
LOCAL_SRC_FILES := \
	debug.c \
	platform.c \
        bst.c \
        service.c \
	input.c \
	output.c \
	ctrl_handler.c \
	af_scaffold.c \
	scaffold_sock.c \
	scaffold_ipv4.c \
	scaffold_udp.c \
	scaffold_tcp.c \
	userlevel/dst.c \
	userlevel/dev.c \
	userlevel/sock.c \
	userlevel/socket.c \
	userlevel/skbuff.c \
	userlevel/timer.c \
	userlevel/wait.c \
	userlevel/client_msg.c \
	userlevel/client.c \
	userlevel/packet_linux.c \
	userlevel/ctrl.c \
	userlevel/scaffold.c

SCAFFOLD_INCLUDE_DIR=$(LOCAL_PATH)/../../include

SCAFFOLD_HDR = \
	af_scaffold.h \
	ctrl.h \
	bst.h \
        service.h \
	input.h \
	output.h \
	scaffold_sock.h \
	scaffold_udp_sock.h \
	scaffold_tcp_sock.h \
	scaffold_ipv4.h \
	userlevel/packet.h \
	userlevel/client_msg.h \
	userlevel/client.h \
	$(SCAFFOLD_INCLUDE_DIR)/libstack/ctrlmsg.h \
	$(SCAFFOLD_INCLUDE_DIR)/netinet/scaffold.h \
	$(SCAFFOLD_INCLUDE_DIR)/scaffold/platform_tcpip.h \
	$(SCAFFOLD_INCLUDE_DIR)/scaffold/platform.h \
	$(SCAFFOLD_INCLUDE_DIR)/scaffold/atomic.h \
	$(SCAFFOLD_INCLUDE_DIR)/scaffold/list.h \
	$(SCAFFOLD_INCLUDE_DIR)/scaffold/hash.h \
	$(SCAFFOLD_INCLUDE_DIR)/scaffold/debug.h \
	$(SCAFFOLD_INCLUDE_DIR)/scaffold/lock.h \
	$(SCAFFOLD_INCLUDE_DIR)/scaffold/net.h \
        $(SCAFFOLD_INCLUDE_DIR)/scaffold/dst.h \
	$(SCAFFOLD_INCLUDE_DIR)/scaffold/netdevice.h \
	$(SCAFFOLD_INCLUDE_DIR)/scaffold/sock.h \
	$(SCAFFOLD_INCLUDE_DIR)/scaffold/skbuff.h \
	$(SCAFFOLD_INCLUDE_DIR)/scaffold/timer.h \
	$(SCAFFOLD_INCLUDE_DIR)/scaffold/wait.h

LOCAL_C_INCLUDES += \
	$(SCAFFOLD_INCLUDE_DIR)

#LOCAL_SHARED_LIBRARIES := \


# We need to compile our own version of libxml2, because the static
# library provided in Android does not have the configured options we need.
LOCAL_LDLIBS :=-lpthread -lrt

LOCAL_SHARED_LIBRARIES +=libdl

EXTRA_DEFINES:=-DHAVE_CONFIG -DOS_ANDROID -DENABLE_DEBUG
LOCAL_CFLAGS:=-O2 -g $(EXTRA_DEFINES)
LOCAL_CPPFLAGS +=$(EXTRA_DEFINES)

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE := scaffold

# LOCAL_UNSTRIPPED_PATH := $(TARGET_ROOT_OUT_BIN_UNSTRIPPED)

include $(BUILD_EXECUTABLE)
