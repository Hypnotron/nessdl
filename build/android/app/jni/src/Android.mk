LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include

# Add your application source files here...
ALL_CPP_FILES := $(wildcard $(LOCAL_PATH)/*.cpp)
ALL_CPP_FILES := $(ALL_CPP_FILES:$(LOCAL_PATH)/%=%)
LOCAL_SRC_FILES := $(ALL_CPP_FILES) 
LOCAL_CPPFLAGS := -std=c++11 -DOS_ANDROID 

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
