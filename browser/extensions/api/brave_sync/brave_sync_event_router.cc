/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/extensions/api/brave_sync/brave_sync_event_router.h"
#include "brave/common/extensions/api/brave_sync.h"
#include "extensions/browser/extension_event_histogram_value.h"
#include "chrome/browser/profiles/profile.h"

namespace extensions {

BraveSyncEventRouter::BraveSyncEventRouter(Profile* profile) : profile_(profile) {
  ;
}

BraveSyncEventRouter::~BraveSyncEventRouter() {
  ;
}

void BraveSyncEventRouter::BrowserToBackgroundPage(const std::string &arg1) {
   if (!profile_) {
     LOG(ERROR) << "TAGAB BraveSyncEventRouter::BrowserToBackgroundPage profiel is not set";
     return;
   }

   EventRouter* event_router = EventRouter::Get(profile_);

   if (event_router) {
    std::unique_ptr<base::ListValue> args(
        extensions::api::brave_sync::OnBrowserToBackgroundPage::Create(arg1)
          .release());
    std::unique_ptr<Event> event(
        new Event(extensions::events::BRAVE_SYNC_BROWSER_TO_BACKGROUND_PAGE,
          extensions::api::brave_sync::OnBrowserToBackgroundPage::kEventName,
          std::move(args)));
    event_router->BroadcastEvent(std::move(event));
  }
}

} // namespace extensions
