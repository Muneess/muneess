BUILD_PAND := true
ifeq ($(BUILD_PAND),true)

LOCAL_PATH:= $(call my-dir)

#
# pand
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	pand.c bnep.c sdp.c

LOCAL_CFLAGS:= \
	-DVERSION=\"4.69\" -DSTORAGEDIR=\"/data/misc/bluetoothd\" -DNEED_PPOLL -D__ANDROID__

LOCAL_C_INCLUDES:=\
	$(LOCAL_PATH)/../lib \
	$(LOCAL_PATH)/../src \

LOCAL_SHARED_LIBRARIES := \
	libbluetoothd \
	libbluetooth \
	libcutils

#LOCAL_MODULE_TAGS :=
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:=pand

include $(BUILD_EXECUTABLE)
endif


#
# hidd
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
        hidd.c \
        sdp.c \
        fakehid.c

LOCAL_CFLAGS:= \
    -DVERSION=\"4.69\" -DSTORAGEDIR=\"/data/misc/bluetoothd\" -DNEED_PPOLL -D__ANDROID__

LOCAL_C_INCLUDES:= \
    $(LOCAL_PATH)/../lib \
    $(LOCAL_PATH)/../src \
    $(LOCAL_PATH)/../gdbus \
    $(call include-path-for, glib) \
    $(call include-path-for, dbus)

LOCAL_SHARED_LIBRARIES := \
    libbluetoothd \
    libbluetooth \
    libdbus \
    libexpat \
    libcutils


#LOCAL_C_INCLUDES:= \
#        $(call include-path-for, bluez-libs) \
#        $(call include-path-for, bluez-utils)/common

#LOCAL_SHARED_LIBRARIES := \
#        libbluetooth

#LOCAL_STATIC_LIBRARIES := \
#        libbluez-utils-common-static

LOCAL_MODULE_PATH := $(TARGET_OUT_OPTIONAL_EXECUTABLES)
#LOCAL_MODULE_TAGS := user eng
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE:=hidd

include $(BUILD_EXECUTABLE)


