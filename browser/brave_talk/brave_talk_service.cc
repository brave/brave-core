#include "brave/browser/brave_talk/brave_talk_service.h"

#include "content/public/browser/web_contents.h"
#include "content/public/browser/navigation_handle.h"

namespace brave_talk {

BraveTalkService::BraveTalkService() = default;
BraveTalkService::~BraveTalkService() {
  // TODO: Remove self from all observers.
  Shutdown();
}

void BraveTalkService::StartObserving(content::WebContents *contents) {
    if (observing_) {
        StopObserving(observing_.get());
    }

    LOG(ERROR) << "Started watching: " << contents;
    // TODO: We should be able to observe multiple web contents.
    observing_ = contents->GetWeakPtr();
    Observe(contents);
}

void BraveTalkService::StopObserving(content::WebContents *contents) {
    LOG(ERROR) << "Stopped watching: " << contents;
    observing_ = nullptr;
    Observe(nullptr);
}

void BraveTalkService::DidStartNavigation(content::NavigationHandle *handle) {
    if (!handle->IsInMainFrame()) return;

    // On any navigation of the main frame stop observing the web contents.
    StopObserving(handle->GetWebContents());
}

}  // namespace brave_talk