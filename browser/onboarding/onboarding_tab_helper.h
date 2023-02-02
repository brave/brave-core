/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ONBOARDING_ONBOARDING_TAB_HELPER_H_
#define BRAVE_BROWSER_ONBOARDING_ONBOARDING_TAB_HELPER_H_

#include <string>

#include "brave/browser/ui/brave_shields_data_controller.h"
#include "components/permissions/permission_request_manager.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class OnboardingTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<OnboardingTabHelper>,
      public permissions::PermissionRequestManager::Observer {
 public:
  static void MaybeCreateForWebContents(content::WebContents* contents);
  OnboardingTabHelper(const OnboardingTabHelper&) = delete;
  OnboardingTabHelper& operator=(const OnboardingTabHelper&) = delete;
  ~OnboardingTabHelper() override;

 private:
  friend class content::WebContentsUserData<OnboardingTabHelper>;

  explicit OnboardingTabHelper(content::WebContents* web_contents);

  // content::WebContentsObserver
  void DidStopLoading() override;
  void WebContentsDestroyed() override;

  // PermissionRequestManager::Observer
  void OnPromptAdded() override;
  void OnPromptRemoved() override;

  void PerformBraveShieldsChecksAndShowHelpBubble();
  bool CanHighlightBraveShields();
  void ShowBraveHelpBubbleView();
  std::string GetTextForOnboardingShieldsBubble();

  raw_ptr<permissions::PermissionRequestManager> permission_request_manager_ =
      nullptr;
  bool browser_has_active_bubble_ = false;
  base::OneShotTimer timer_delay_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_BROWSER_ONBOARDING_ONBOARDING_TAB_HELPER_H_
