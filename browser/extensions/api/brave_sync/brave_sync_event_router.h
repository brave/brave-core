/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SYNC_BRAVE_SYNC_EVENT_ROUTER_H
#define BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SYNC_BRAVE_SYNC_EVENT_ROUTER_H

#include <string>
#include "extensions/browser/event_router.h"

class Profile;

namespace extensions {

class BraveSyncEventRouter {
public:
  BraveSyncEventRouter(Profile* profile);
  ~BraveSyncEventRouter();

  void BrowserToBackgroundPage(const std::string &arg1);

  void BrowserToBackgroundPageRaw(const std::string &message,
    const base::Value &arg1,
    const base::Value &arg2,
    const base::Value &arg3,
    const base::Value &arg4);

private:
  Profile* profile_;
};

} // namespace extensions

#endif // BRAVE_BROWSER_EXTENSIONS_API_BRAVE_SYNC_BRAVE_SYNC_EVENT_ROUTER_H
