/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_TAB_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_TAB_HELPER_H_

#include <optional>
#include <string>

#include "base/functional/callback_forward.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wayback_machine/wayback_machine_url_fetcher.h"
#include "brave/components/brave_wayback_machine/wayback_state.h"
#include "components/prefs/pref_member.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "ui/gfx/native_widget_types.h"

class PrefService;

class BraveWaybackMachineTabHelper
    : public content::WebContentsObserver,
      public WaybackMachineURLFetcher::Client,
      public content::WebContentsUserData<BraveWaybackMachineTabHelper> {
 public:
  static void CreateIfNeeded(content::WebContents* web_contents);

  using WaybackStateChangedCallback =
      base::RepeatingCallback<void(WaybackState state)>;

  ~BraveWaybackMachineTabHelper() override;

  BraveWaybackMachineTabHelper(const BraveWaybackMachineTabHelper&) = delete;
  BraveWaybackMachineTabHelper& operator=(
      const BraveWaybackMachineTabHelper&) = delete;

  void SetWaybackStateChangedCallback(WaybackStateChangedCallback callback);
  void set_active_window(gfx::NativeWindow window) { active_window_ = window; }
  gfx::NativeWindow active_window() const { return active_window_; }
  WaybackState wayback_state() const { return wayback_state_; }

  void FetchWaybackURL();

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveWaybackMachineTest, BubbleLaunchTest);

  explicit BraveWaybackMachineTabHelper(content::WebContents* contents);

  // content::WebContentsObserver overrides:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  // WaybackMachineURLFetcher::Client overrides:
  void OnWaybackURLFetched(const GURL& latest_wayback_url) override;

  void SetWaybackState(WaybackState state);
  bool ShouldCheckWaybackMachine(int response_code) const;
  void OnWaybackEnabledChanged(const std::string& pref_name);
  void ResetState();

  // Cache wayback url navigation handle.
  // It uses to check whether it's wayback url loading or not.
  // If it's wayback url loading from previous navigation,
  // we should not touch wayback state.
  std::optional<int64_t> wayback_url_navigation_id_;

  // If not null, this tab has active window.
  gfx::NativeWindow active_window_ = nullptr;
  WaybackState wayback_state_ = WaybackState::kInitial;
  WaybackStateChangedCallback wayback_state_changed_callback_;
  raw_ref<PrefService> pref_service_;
  WaybackMachineURLFetcher wayback_machine_url_fetcher_;
  BooleanPrefMember wayback_enabled_;

  friend WebContentsUserData;
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_TAB_HELPER_H_
