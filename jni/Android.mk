LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := esp
LOCAL_SRC_FILES := main.cpp
LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_CPPFLAGS := -std=c++17 -fno-rtti -fno-exceptions -Oz -fvisibility=hidden
LOCAL_LDFLAGS := -Wl,--gc-sections -Wl,--strip-all -pie
LOCAL_LDLIBS := -llog -landroid -lEGL -lGLESv3
include $(BUILD_EXECUTABLE)
