/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ONBOARDING_ONBOARDING_TAB_HELPER_H_
#define BRAVE_BROWSER_ONBOARDING_ONBOARDING_TAB_HELPER_H_

#include <memory>
#include <string>

#include "base/auto_reset.h"
#include "base/functional/callback.h"
#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"

class PrefRegistrySimple;

namespace content {
class WebContents;
}  // namespace content

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace onboarding {
void RegisterLocalStatePrefs(PrefRegistrySimple* registry);
}  // namespace onboarding

// Highlights the Brave Shields icon once, for users who installed Brave in the
// last seven days. Owned by BraveTabFeatures.
class OnboardingTabHelper : public tabs::ContentsObservingTabFeature {
 public:
  using WebContentsCallback =
      base::RepeatingCallback<void(content::WebContents*)>;

  // Returns nullptr when the highlight has already been shown.
  static std::unique_ptr<OnboardingTabHelper> MaybeCreate(
      tabs::TabInterface& tab);

  explicit OnboardingTabHelper(tabs::TabInterface& tab);
  OnboardingTabHelper(const OnboardingTabHelper&) = delete;
  OnboardingTabHelper& operator=(const OnboardingTabHelper&) = delete;
  ~OnboardingTabHelper() override;

  // Sets a callback that runs, with the tab's contents, once a helper's
  // first-run check has completed, so that it may show the highlight. It
  // applies to every tab until the returned value is destroyed.
  [[nodiscard]] static base::AutoReset<WebContentsCallback>
  SetCanHighlightCallbackForTesting(WebContentsCallback callback);

  // Sets a callback that runs, with the tab's contents, each time loading stops
  // in a tab with a helper, once the helper has considered the highlight. It
  // applies to every tab until the returned value is destroyed.
  [[nodiscard]] static base::AutoReset<WebContentsCallback>
  SetDidStopLoadingCallbackForTesting(WebContentsCallback callback);

 private:
  friend class OnboardingBrowserTest;
  friend class OnboardingTest;
  FRIEND_TEST_ALL_PREFIXES(OnboardingTest, HelperCreationTestForFirstRun);
  FRIEND_TEST_ALL_PREFIXES(OnboardingTest, HelperCreationTestForNonFirstRun);

  // tabs::ContentsObservingTabFeature:
  void OnDiscardContents(tabs::TabInterface* tab,
                         content::WebContents* old_contents,
                         content::WebContents* new_contents) override;

  // content::WebContentsObserver:
  void DidStopLoading() override;

  void MaybeHighlightShields();

  void SetCanHighlight();
  void OnSevenDaysPassedSinceFirstRunChecked(bool passed);
  void PerformBraveShieldsChecksAndShowHelpBubble();
  bool CanHighlightBraveShields();
  void ShowBraveHelpBubbleView();
  std::string GetTextForOnboardingShieldsBubble();
  void CleanUp();

  static bool IsSevenDaysPassedSinceFirstRun();

  // Whether the highlight may be shown in this tab. False until the first-run
  // check completes, and once the helper has cleaned up.
  bool can_highlight_ = false;
  // Set once the helper no longer observes the tab's contents, including any
  // that replace them.
  bool cleaned_up_ = false;

  base::WeakPtrFactory<OnboardingTabHelper> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_ONBOARDING_ONBOARDING_TAB_HELPER_H_
