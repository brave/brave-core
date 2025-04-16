/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wayback_machine/brave_wayback_machine_tab_helper.h"

#include <utility>

#include "base/check_is_test.h"
#include "base/command_line.h"
#include "base/containers/fixed_flat_set.h"
#include "base/containers/flat_set.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/task/sequenced_task_runner.h"
#include "brave/components/brave_wayback_machine/brave_wayback_machine_utils.h"
#include "brave/components/brave_wayback_machine/pref_names.h"
#include "brave/components/constants/brave_switches.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"

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

BraveWaybackMachineTabHelper::~BraveWaybackMachineTabHelper() {
  CHECK(!wayback_state_changed_callback_);
}

void BraveWaybackMachineTabHelper::FetchWaybackURL() {
  CHECK(wayback_enabled_.GetValue());
  SetWaybackState(WaybackState::kFetching);
  wayback_machine_url_fetcher_.Fetch(web_contents()->GetVisibleURL());
}

void BraveWaybackMachineTabHelper::SetWaybackStateChangedCallback(
    WaybackStateChangedCallback callback) {
  // callback should be set only once.
  if (callback) {
    // Some upstream tests (ex, *.TestGroupDetachedAndReInserted)
    // do tab group detach & re-attach by raw api w/o updating active tab state
    // that happens in product.
    // In production, let say we have 4 tabs (tab A, B, C and D) in window A.
    // made a tab group with tab A and B. and Current active tab is D.
    // When that tab group is dragged and detached, tab A becomes active tab
    // during the dragging(before detach) and active tab could be
    // tab C or D in window A after detached. Then, new window B is created
    // after tab group is detached. And tab A(or B) could become active tab in
    // window B. If that tab group in window B is detached, tab A(in window B)
    // becomes as inactive tab and it becomes active tab in window A after that
    // tab group is attached to window A. This is a tab activation flow in
    // production. In the test(ex,
    // TabGroupsApiTest.TestGroupDetachedAndReInserted), that tab group is
    // detached by calling DetachTabGroupForInsertion(group). During that
    // detaching, any tab activation change signal is not delivered because this
    // test omits tab group dragging step before detaching. So, tab D is still
    // active Tab after calling that api. And then, this tab group is
    // re-inserted by calling InsertDetachedTabGroupAt(). When this happens,
    // window A gets active tab changed signal via OnTabStripModelChanged(). and
    // |selection| args delivered by OnTabStripModelChanged() gives true for
    // `active_tab_changed()`. and |selection.new_contents| points to D. Because
    // of this tab D gets activated signal twice. Or `active_tab_changed()`
    // should give false if active tab is still D. In production, tab A is
    // active tab instead of tab D. IMO, that upstream test should be improved.
    if (wayback_state_changed_callback_) {
      CHECK_IS_TEST();
    }
  }

  wayback_state_changed_callback_ = std::move(callback);
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

  if (IsWaybackMachineDisabledFor(navigation_handle->GetURL())) {
    return;
  }

  // Double check with user visible url to check user visible only schemes such
  // as "view-source:"
  if (IsWaybackMachineDisabledFor(web_contents()->GetLastCommittedURL())) {
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
    const GURL& latest_wayback_url) {
  // Ignore the result if disabled.
  if (!wayback_enabled_.GetValue()) {
    return;
  }

  // wayback url is not available.
  if (latest_wayback_url.is_empty()) {
    SetWaybackState(WaybackState::kNotAvailable);
    return;
  }

  SetWaybackState(WaybackState::kLoaded);

  if (auto navigation_handle = web_contents()->GetController().LoadURL(
          latest_wayback_url, content::Referrer(), ui::PAGE_TRANSITION_LINK,
          std::string())) {
    wayback_url_navigation_id_ = navigation_handle->GetNavigationId();
  }
}

void BraveWaybackMachineTabHelper::SetWaybackState(WaybackState state) {
  if (wayback_state_ == state) {
    return;
  }

  wayback_state_ = state;

  if (wayback_state_changed_callback_) {
    wayback_state_changed_callback_.Run(wayback_state_);
  }
}

bool BraveWaybackMachineTabHelper::ShouldCheckWaybackMachine(
    int response_code) const {
  static constexpr auto responses = base::MakeFixedFlatSet<int>({
      net::HTTP_NOT_FOUND,              // 404
      net::HTTP_REQUEST_TIMEOUT,        // 408
      net::HTTP_GONE,                   // 410
      451,                              // Unavailable For Legal Reasons
      net::HTTP_INTERNAL_SERVER_ERROR,  // 500
      net::HTTP_BAD_GATEWAY,            // 502,
      net::HTTP_SERVICE_UNAVAILABLE,    // 503,
      net::HTTP_GATEWAY_TIMEOUT,        // 504,
      509,                              // Bandwidth Limit Exceeded
      520,                              // Web Server Returned an Unknown Error
      521,                              // Web Server Is Down
      523,                              // Origin Is Unreachable
      524,                              // A Timeout Occurred
      525,                              // SSL Handshake Failed
      526                               // Invalid SSL Certificate
  });

  return responses.contains(response_code);
}

void BraveWaybackMachineTabHelper::OnWaybackEnabledChanged(
    const std::string& pref_name) {
  // Back to initial state when user disables this feature.
  if (!wayback_enabled_.GetValue()) {
    ResetState();
  }
}

void BraveWaybackMachineTabHelper::ResetState() {
  wayback_url_navigation_id_ = std::nullopt;
  SetWaybackState(WaybackState::kInitial);
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(BraveWaybackMachineTabHelper);
