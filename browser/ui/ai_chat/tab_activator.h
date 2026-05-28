// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_AI_CHAT_TAB_ACTIVATOR_H_
#define BRAVE_BROWSER_UI_AI_CHAT_TAB_ACTIVATOR_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "components/keyed_service/core/keyed_service.h"

class Profile;

namespace ai_chat {

class TabTrackerService;

// Installs an activator callback into the profile's TabTrackerService so that
// Leo can switch the user to a tab by id. The activator iterates the browsers
// belonging to `profile` and asks the matching TabStripModel to activate the
// tab.
//
// This class only exists in `brave/browser/ui/ai_chat` so that the activation
// implementation can pull in `chrome/browser/ui` (BrowserList, TabStripModel)
// without forcing those deps onto `brave/browser/ai_chat`, which would create
// a GN dependency cycle.
class TabActivator : public KeyedService {
 public:
  TabActivator(Profile* profile, TabTrackerService* tab_tracker);
  ~TabActivator() override;

  TabActivator(const TabActivator&) = delete;
  TabActivator& operator=(const TabActivator&) = delete;

 private:
  bool ActivateTab(int32_t tab_id);

  raw_ptr<Profile> profile_;
  raw_ptr<TabTrackerService> tab_tracker_;
  base::WeakPtrFactory<TabActivator> weak_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_AI_CHAT_TAB_ACTIVATOR_H_
