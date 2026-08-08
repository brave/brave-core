// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_AI_CHAT_TAB_ACTIVATOR_H_
#define BRAVE_BROWSER_UI_AI_CHAT_TAB_ACTIVATOR_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/ai_chat/core/browser/tab_tracker_service.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

namespace ai_chat {

// Acts as the profile's TabTrackerService delegate so that Leo can switch the
// user to a tab by id. Resolves the id with `tabs::TabHandle` and asks that
// tab's TabStripModel to activate it, ignoring tabs outside `profile`.
//
// Lives in brave/browser/ui/ai_chat (not brave/browser/ai_chat) so the
// activator can use chrome/browser/ui's BrowserList / TabStripModel without a
// GN dependency cycle.
class TabActivator : public KeyedService, public TabTrackerService::Delegate {
 public:
  TabActivator(Profile* profile, TabTrackerService* tab_tracker);
  ~TabActivator() override;

  TabActivator(const TabActivator&) = delete;
  TabActivator& operator=(const TabActivator&) = delete;

 private:
  // TabTrackerService::Delegate:
  bool ActivateTab(int32_t tab_id) override;

  raw_ptr<Profile> profile_;
  raw_ptr<TabTrackerService> tab_tracker_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_AI_CHAT_TAB_ACTIVATOR_H_
