// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_ENABLED_STATE_TRANSITION_SERVICE_H_
#define BRAVE_BROWSER_AI_CHAT_ENABLED_STATE_TRANSITION_SERVICE_H_

#include "base/memory/raw_ptr.h"
#include "build/build_config.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;
class Profile;

namespace content {
class BrowserContext;
}

#if !BUILDFLAG(IS_ANDROID)
namespace sidebar {
class SidebarService;
}
#endif

namespace ai_chat {

class EnabledStateTransitionService : public KeyedService {
 public:
  explicit EnabledStateTransitionService(content::BrowserContext* context);
  ~EnabledStateTransitionService() override;

  EnabledStateTransitionService(const EnabledStateTransitionService&) = delete;
  EnabledStateTransitionService& operator=(
      const EnabledStateTransitionService&) = delete;

 private:
  void OnEnabledByPolicyChanged();
#if !BUILDFLAG(IS_ANDROID)
  void CloseAIChatTabs();
  void UpdateSidebarState(bool enabled);
#endif

  raw_ptr<Profile> profile_;
#if !BUILDFLAG(IS_ANDROID)
  raw_ptr<sidebar::SidebarService> sidebar_service_;
#endif
  raw_ptr<PrefService> prefs_;
  PrefChangeRegistrar pref_change_registrar_;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_ENABLED_STATE_TRANSITION_SERVICE_H_
