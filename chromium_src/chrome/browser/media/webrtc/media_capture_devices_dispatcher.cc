#include "brave/browser/brave_talk/brave_talk_media_access_handler.h"

#define BRAVE_MEDIA_CAPTURE_DEVICES_DISPATCHER media_access_handlers_.push_back(std::make_unique<brave_talk::BraveTalkMediaAccessHandler>());

#include "src/chrome/browser/media/webrtc/media_capture_devices_dispatcher.cc"

#undef BRAVE_MEDIA_CAPTURE_DEVICES_DISPATCHER