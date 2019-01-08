LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := project
LOCAL_SRC_FILES := Project.cpp
LOCAL_LDLIBS :=-llog
LOCAL_CFLAGS := -fvisibility=hidden

include $(BUILD_SHARED_LIBRARY)