#include "brave/browser/brave_talk/brave_talk_service.h"

#include "content/public/browser/web_contents.h"

namespace brave_talk {

BraveTalkService::BraveTalkService() = default;
BraveTalkService::~BraveTalkService() {
  // TODO: Remove self from all observers.
  Shutdown();
}

void BraveTalkService::StartObserving(content::WebContents *contents) {
    LOG(ERROR) << "Started watching: " << contents;
}

void BraveTalkService::StopObserving(content::WebContents *contents) {
    LOG(ERROR) << "Stopped watching: " << contents;
}

}  // namespace brave_talk