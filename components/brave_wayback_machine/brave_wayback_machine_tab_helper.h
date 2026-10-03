/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_TAB_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_TAB_HELPER_H_

#include <optional>
#include <string>

#include "base/callback_list.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "brave/components/brave_wayback_machine/wayback_machine_url_fetcher.h"
#include "brave/components/brave_wayback_machine/wayback_state.h"
#include "components/prefs/pref_member.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "url/gurl.h"

class PrefService;

class BraveWaybackMachineTabHelper
    : public content::WebContentsObserver,
      public WaybackMachineURLFetcher::Client,
      public content::WebContentsUserData<BraveWaybackMachineTabHelper> {
 public:
  static void CreateIfNeeded(content::WebContents* web_contents);

  using WaybackStateChangedCallbackList =
      base::RepeatingCallbackList<void(WaybackState state)>;
  using WaybackStateChangedCallback =
      WaybackStateChangedCallbackList::CallbackType;

  ~BraveWaybackMachineTabHelper() override;

  BraveWaybackMachineTabHelper(const BraveWaybackMachineTabHelper&) = delete;
  BraveWaybackMachineTabHelper& operator=(
      const BraveWaybackMachineTabHelper&) = delete;

  // Registers a callback invoked when the WaybackState changes. Destroying the
  // returned subscription unregisters the callback.
  base::CallbackListSubscription RegisterWaybackStateChangedCallback(
      WaybackStateChangedCallback callback);

  // Returns the current WaybackState.
  WaybackState wayback_state() const { return wayback_state_; }

  // Sets the wayback state directly and notifies registered callbacks,
  // bypassing navigation and the real wayback-machine lookup.
  void SetWaybackStateForTesting(WaybackState state) { SetWaybackState(state); }

  // Initiates fetching the latest snapshot URL for the current page. The
  // snapshot is loaded as soon as it is found.
  void FetchWaybackURL();

  // Loads the snapshot found by an automatic check. Must only be called in the
  // kFound state.
  void LoadWaybackURL();

  // Returns the snapshot URL found by the last fetch, if any.
  const GURL& wayback_url() const { return wayback_url_; }

  // Returns the time of the snapshot found by the last fetch. Null if unknown.
  base::Time snapshot_time() const { return snapshot_time_; }

 private:
  explicit BraveWaybackMachineTabHelper(content::WebContents* contents);

  // content::WebContentsObserver overrides:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  // WaybackMachineURLFetcher::Client overrides:
  void OnWaybackURLFetched(const GURL& latest_wayback_url,
                           base::Time snapshot_time) override;

  void StartFetch(bool is_auto_check);
  bool ShouldAutoCheck() const;
  void NavigateToWaybackURL();
  void SetWaybackState(WaybackState state);
  void OnWaybackEnabledChanged(const std::string& pref_name);
  void ResetState();

  // Cache wayback url navigation handle.
  // It uses to check whether it's wayback url loading or not.
  // If it's wayback url loading from previous navigation,
  // we should not touch wayback state.
  std::optional<int64_t> wayback_url_navigation_id_;

  GURL wayback_url_;
  base::Time snapshot_time_;
  bool is_auto_check_ = false;

  WaybackState wayback_state_ = WaybackState::kInitial;
  WaybackStateChangedCallbackList wayback_state_changed_callbacks_;
  raw_ref<PrefService> pref_service_;
  WaybackMachineURLFetcher wayback_machine_url_fetcher_;
  BooleanPrefMember wayback_enabled_;

  friend WebContentsUserData;
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_TAB_HELPER_H_
