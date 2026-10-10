/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"

#include <utility>

#include "base/auto_reset.h"
#include "base/check.h"
#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_utils.h"
#include "brave/components/brave_wayback_machine/pref_names.h"
#include "brave/components/constants/brave_switches.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "net/http/http_response_headers.h"

// static
void BraveWaybackMachineTabHelper::CreateIfNeeded(
    content::WebContents* web_contents) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableBraveWaybackMachineExtension)) {
    return;
  }

  BraveWaybackMachineTabHelper::CreateForWebContents(web_contents);
}

BraveWaybackMachineTabHelper::BraveWaybackMachineTabHelper(
    content::WebContents* contents)
    : WebContentsObserver(contents),
      content::WebContentsUserData<BraveWaybackMachineTabHelper>(*contents),
      pref_service_(*user_prefs::UserPrefs::Get(contents->GetBrowserContext())),
      wayback_machine_url_fetcher_(
          this,
          contents->GetBrowserContext()
              ->GetDefaultStoragePartition()
              ->GetURLLoaderFactoryForBrowserProcess()) {
  // Unretained() is safe as |wayback_enabled_| is owned by this class.
  wayback_enabled_.Init(
      kBraveWaybackMachineEnabled, &*pref_service_,
      base::BindRepeating(
          &BraveWaybackMachineTabHelper::OnWaybackEnabledChanged,
          base::Unretained(this)));

}

BraveWaybackMachineTabHelper::~BraveWaybackMachineTabHelper() = default;

void BraveWaybackMachineTabHelper::SetWaybackStateForTesting(
    WaybackState state) {
  CHECK_NE(state, WaybackState::kFound);
  if (state != WaybackState::kLoaded) {
    snapshot_info_.reset();
  }
  SetWaybackState(state);
}

void BraveWaybackMachineTabHelper::SetFoundForTesting(
    WaybackSnapshotInfo snapshot) {
  snapshot_info_ = std::move(snapshot);
  SetWaybackState(WaybackState::kFound);
}

void BraveWaybackMachineTabHelper::FetchSnapshotInfo() {
  CHECK(wayback_enabled_.GetValue());
  snapshot_info_.reset();
  SetWaybackState(WaybackState::kFetching);
  wayback_machine_url_fetcher_.Fetch(web_contents()->GetVisibleURL());
}

void BraveWaybackMachineTabHelper::NavigateToSnapshot() {
  CHECK_EQ(wayback_state_, WaybackState::kFound);
  const GURL url = snapshot_info_->url;
  SetWaybackState(WaybackState::kLoaded);

  if (auto navigation_handle = web_contents()->GetController().LoadURL(
          url, content::Referrer(), ui::PAGE_TRANSITION_LINK, std::string())) {
    wayback_url_navigation_id_ = navigation_handle->GetNavigationId();
  }
}

base::CallbackListSubscription
BraveWaybackMachineTabHelper::RegisterWaybackStateChangedCallback(
    WaybackStateChangedCallback callback) {
  return wayback_state_changed_callbacks_.Add(std::move(callback));
}

void BraveWaybackMachineTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!wayback_enabled_.GetValue()) {
    ResetState();
    return;
  }

  if (!navigation_handle->IsInPrimaryMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  // Don't reset current state if it's wayback url navigation.
  // Otherwise, we lost kLoaded state after loading it.
  if (wayback_url_navigation_id_ &&
      wayback_url_navigation_id_ == navigation_handle->GetNavigationId()) {
    wayback_url_navigation_id_ = std::nullopt;
    return;
  }

  ResetState();

  if (!IsWaybackMachineEnabledFor(navigation_handle->GetURL())) {
    return;
  }

  // Double check with user visible url to check user visible only schemes such
  // as "view-source:"
  if (!IsWaybackMachineEnabledFor(web_contents()->GetLastCommittedURL())) {
    return;
  }

  const net::HttpResponseHeaders* header =
      navigation_handle->GetResponseHeaders();
  if (!header) {
    return;
  }

  if (ShouldCheckWaybackMachine(header->response_code())) {
    SetWaybackState(WaybackState::kNeedToCheck);
  }
}

void BraveWaybackMachineTabHelper::OnWaybackURLFetched(
    std::optional<WaybackSnapshotInfo> snapshot) {
  // Ignore the result if disabled.
  if (!wayback_enabled_.GetValue()) {
    return;
  }

  if (!snapshot) {
    SetWaybackState(WaybackState::kNotAvailable);
    return;
  }

  snapshot_info_ = std::move(snapshot);
  SetWaybackState(WaybackState::kFound);
}

void BraveWaybackMachineTabHelper::SetWaybackState(WaybackState state) {
  if (wayback_state_ == state) {
    return;
  }

  wayback_state_ = state;

  // Avoid reentrancy (and the out-of-order notifications that can result) by
  // queueing state notifications. Iteratively flush the queue after each
  // notification.
  pending_state_notifications_.push(state);
  if (notifying_state_changed_) {
    return;
  }

  base::AutoReset<bool> notifying(&notifying_state_changed_, true);
  while (!pending_state_notifications_.empty()) {
    const WaybackState next = pending_state_notifications_.front();
    pending_state_notifications_.pop();
    wayback_state_changed_callbacks_.Notify(next);
  }
}

void BraveWaybackMachineTabHelper::OnWaybackEnabledChanged(
    const std::string& pref_name) {
  // Back to initial state when user disables this feature.
  if (!wayback_enabled_.GetValue()) {
    ResetState();
  }
}

void BraveWaybackMachineTabHelper::ResetState() {
  wayback_machine_url_fetcher_.Cancel();
  wayback_url_navigation_id_ = std::nullopt;
  snapshot_info_.reset();
  SetWaybackState(WaybackState::kInitial);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveWaybackMachineTabHelper);
