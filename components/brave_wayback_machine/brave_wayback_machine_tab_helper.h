/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_TAB_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_TAB_HELPER_H_

#include <optional>
#include <string>

#include "base/callback_list.h"
#include "base/containers/queue.h"
#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wayback_machine/wayback_machine_url_fetcher.h"
#include "brave/components/brave_wayback_machine/wayback_snapshot_info.h"
#include "brave/components/brave_wayback_machine/wayback_state.h"
#include "components/prefs/pref_member.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

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
  // returned subscription unregisters the callback. State changes made from
  // within a callback are delivered, in order, after all callbacks have been
  // notified of the current change.
  base::CallbackListSubscription RegisterWaybackStateChangedCallback(
      WaybackStateChangedCallback callback);

  // Returns the current WaybackState.
  WaybackState wayback_state() const { return wayback_state_; }

  // Returns the snapshot info found by the last lookup. Set only in the kFound
  // and kLoaded states.
  const std::optional<WaybackSnapshotInfo>& snapshot_info() const {
    return snapshot_info_;
  }

  // Sets the wayback state directly and notifies registered callbacks,
  // bypassing navigation and the real wayback-machine lookup.
  void SetWaybackStateForTesting(WaybackState state);

  // Enters the kFound state with |snapshot|.
  void SetFoundForTesting(WaybackSnapshotInfo snapshot);

  // Looks up the latest snapshot of the current page.
  void FetchSnapshotInfo();

  // Navigates to the snapshot found by FetchSnapshotInfo().
  void NavigateToSnapshot();

 private:
  explicit BraveWaybackMachineTabHelper(content::WebContents* contents);

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  // WaybackMachineURLFetcher::Client:
  void OnWaybackURLFetched(
      std::optional<WaybackSnapshotInfo> snapshot) override;

  void SetWaybackState(WaybackState state);
  void OnWaybackEnabledChanged(const std::string& pref_name);
  void ResetState();

  // Cache wayback url navigation handle.
  // It uses to check whether it's wayback url loading or not.
  // If it's wayback url loading from previous navigation,
  // we should not touch wayback state.
  std::optional<int64_t> wayback_url_navigation_id_;

  std::optional<WaybackSnapshotInfo> snapshot_info_;
  WaybackState wayback_state_ = WaybackState::kInitial;
  WaybackStateChangedCallbackList wayback_state_changed_callbacks_;
  base::queue<WaybackState> pending_state_notifications_;
  bool notifying_state_changed_ = false;
  raw_ref<PrefService> pref_service_;
  WaybackMachineURLFetcher wayback_machine_url_fetcher_;
  BooleanPrefMember wayback_enabled_;

  friend WebContentsUserData;
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // BRAVE_COMPONENTS_BRAVE_WAYBACK_MACHINE_BRAVE_WAYBACK_MACHINE_TAB_HELPER_H_
