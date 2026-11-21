/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"

#include <utility>

#include "base/auto_reset.h"
#include "base/check.h"
#include "base/command_line.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_utils.h"
#include "brave/components/brave_wayback_machine/features.h"
#include "brave/components/brave_wayback_machine/pref_names.h"
#include "brave/components/constants/brave_switches.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_interface.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "net/base/ip_address.h"
#include "net/base/ip_endpoint.h"
#include "net/http/http_response_headers.h"
#include "url/gurl.h"

DEFINE_USER_DATA(BraveWaybackMachineTabHelper);

// static
BraveWaybackMachineTabHelper* BraveWaybackMachineTabHelper::From(
    tabs::TabInterface* tab) {
  return tab ? Get(tab->GetUnownedUserDataHost()) : nullptr;
}

// static
std::unique_ptr<BraveWaybackMachineTabHelper>
BraveWaybackMachineTabHelper::MaybeCreate(tabs::TabInterface& tab) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kDisableBraveWaybackMachineExtension)) {
    return nullptr;
  }

  return std::make_unique<BraveWaybackMachineTabHelper>(tab);
}

BraveWaybackMachineTabHelper::BraveWaybackMachineTabHelper(
    tabs::TabInterface& tab)
    : WebContentsObserver(tab.GetContents()),
      tab_(tab),
      pref_service_(
          *user_prefs::UserPrefs::Get(tab.GetContents()->GetBrowserContext())),
      wayback_machine_url_fetcher_(
          this,
          tab.GetContents()
              ->GetBrowserContext()
              ->GetDefaultStoragePartition()
              ->GetURLLoaderFactoryForBrowserProcess()),
      will_discard_contents_subscription_(tab.RegisterWillDiscardContents(
          base::BindRepeating(&BraveWaybackMachineTabHelper::OnDiscardContents,
                              base::Unretained(this)))),
      scoped_unowned_user_data_(tab.GetUnownedUserDataHost(), *this) {
  // Unretained() is safe as |wayback_enabled_| is owned by this class.
  wayback_enabled_.Init(
      kBraveWaybackMachineEnabled, &*pref_service_,
      base::BindRepeating(
          &BraveWaybackMachineTabHelper::OnWaybackEnabledChanged,
          base::Unretained(this)));
}

BraveWaybackMachineTabHelper::~BraveWaybackMachineTabHelper() = default;

bool BraveWaybackMachineTabHelper::CanAutoCheck() const {
  if (wayback_state_ != WaybackState::kNeedToCheck ||
      !has_public_remote_endpoint_) {
    return false;
  }
  if (!base::FeatureList::IsEnabled(
          brave_wayback_machine::features::kWaybackMachineAutoCheck)) {
    return false;
  }
  if (!wayback_enabled_.GetValue() ||
      !pref_service_->GetBoolean(kBraveWaybackMachineAutoCheckEnabled)) {
    return false;
  }
  if (!tab_->IsActivated()) {
    return false;
  }
  content::WebContents* contents = web_contents();
  if (!contents || contents->GetBrowserContext()->IsOffTheRecord()) {
    return false;
  }
  const GURL& url = contents->GetLastCommittedURL();
  return IsWaybackMachineEnabledFor(url) && !url.has_query() &&
         !url.has_username() && !url.has_password();
}

void BraveWaybackMachineTabHelper::FetchSnapshotInfo() {
  CHECK(wayback_enabled_.GetValue());
  snapshot_info_.reset();
  SetWaybackState(WaybackState::kFetching);
  wayback_machine_url_fetcher_.Fetch(web_contents()->GetLastCommittedURL());
}

void BraveWaybackMachineTabHelper::MaybeNavigateToSnapshot() {
  if (!snapshot_info_ || wayback_state_ == WaybackState::kLoaded) {
    return;
  }

  const GURL url = snapshot_info_->url;
  SetWaybackState(WaybackState::kLoaded);

  // LoadURL() fires DidStartNavigation() synchronously before returning.
  base::AutoReset<bool> loading_wayback_url(&loading_wayback_url_, true);
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

void BraveWaybackMachineTabHelper::DidStartNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      navigation_handle->IsSameDocument()) {
    return;
  }

  if (loading_wayback_url_) {
    wayback_url_navigation_id_ = navigation_handle->GetNavigationId();
    return;
  }

  if (wayback_url_navigation_id_ &&
      wayback_url_navigation_id_ == navigation_handle->GetNavigationId()) {
    return;
  }

  ResetState();
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

  if (!navigation_handle->HasCommitted()) {
    return;
  }

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
    const net::IPAddress remote_address =
        navigation_handle->GetSocketAddress().address();
    has_public_remote_endpoint_ =
        remote_address.IsValid() && remote_address.IsPubliclyRoutable();
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
  has_public_remote_endpoint_ = false;
  snapshot_info_.reset();
  SetWaybackState(WaybackState::kInitial);
}

void BraveWaybackMachineTabHelper::OnDiscardContents(
    tabs::TabInterface* tab,
    content::WebContents* old_contents,
    content::WebContents* new_contents) {
  ResetState();
  Observe(new_contents);
}
