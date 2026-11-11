/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/onion_location_tab_helper.h"

#include <memory>
#include <utility>

#include "base/functional/bind.h"
#include "base/memory/ptr_util.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/web_contents_delegate.h"

namespace {

struct OnionLocation final : public base::SupportsUserData::Data {
  ~OnionLocation() final = default;

  static const GURL& Get(content::NavigationEntry* entry) {
    if (auto* data =
            static_cast<OnionLocation*>(entry->GetUserData(kUserDataKey))) {
      return data->onion_location_;
    }
    return GURL::EmptyGURL();
  }

  static void Set(content::NavigationEntry* entry, const GURL& url) {
    if (!entry) {
      return;
    }
    if (url.is_empty()) {
      entry->RemoveUserData(kUserDataKey);
    } else {
      entry->SetUserData(kUserDataKey,
                         base::WrapUnique(new OnionLocation(url)));
    }
  }

 private:
  static constexpr char kUserDataKey[] = "tor_onion_location";
  explicit OnionLocation(const GURL& url) : onion_location_(url) {}

  GURL onion_location_;
};
}  // namespace

namespace tor {

DEFINE_USER_DATA(OnionLocationTabHelper);

// static
OnionLocationTabHelper* OnionLocationTabHelper::From(tabs::TabInterface* tab) {
  return tab ? Get(tab->GetUnownedUserDataHost()) : nullptr;
}

OnionLocationTabHelper::OnionLocationTabHelper(tabs::TabInterface& tab)
    : content::WebContentsObserver(tab.GetContents()),
      will_discard_contents_subscription_(tab.RegisterWillDiscardContents(
          base::BindRepeating(&OnionLocationTabHelper::OnDiscardContents,
                              base::Unretained(this)))),
      scoped_unowned_user_data_(tab.GetUnownedUserDataHost(), *this) {}

OnionLocationTabHelper::~OnionLocationTabHelper() = default;

// static
void OnionLocationTabHelper::SetOnionLocationByThrottle(
    content::WebContents* web_contents,
    const GURL& onion_location,
    const std::optional<url::Origin>& initiator_origin) {
  auto* tab_helper = OnionLocationTabHelper::From(
      tabs::TabInterface::MaybeGetFromContents(web_contents));
  if (!tab_helper) {
    return;
  }
  tab_helper->throttle_reported_onion_location_ = onion_location;
  tab_helper->throttle_reported_initiator_origin_ =
      onion_location.is_empty() ? std::nullopt : initiator_origin;
}

void OnionLocationTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }
  auto* entry = navigation_handle->GetNavigationEntry();
  if (!entry) {
    // Throttle could block the request and set up the onion location if user
    // tried to open .onion domain in the normal window. In this case we should
    // show the error page with .onion location.
    onion_location_ = throttle_reported_onion_location_;
    initiator_origin_ = throttle_reported_initiator_origin_;
  } else if (navigation_handle->IsServedFromBackForwardCache()) {
    onion_location_ = OnionLocation::Get(entry);
    initiator_origin_.reset();
  } else {
    onion_location_ = throttle_reported_onion_location_;
    initiator_origin_.reset();
    OnionLocation::Set(entry, throttle_reported_onion_location_);
  }
  if (auto* delegate = web_contents()->GetDelegate()) {
    // This forces the page action to update the ui state.
    delegate->NavigationStateChanged(
        web_contents(), content::InvalidateTypes::INVALIDATE_TYPE_URL);
  }
}

void OnionLocationTabHelper::OnDiscardContents(
    tabs::TabInterface* tab,
    content::WebContents* old_contents,
    content::WebContents* new_contents) {
  // The Onion-Location belongs to the old contents' page.
  throttle_reported_onion_location_ = GURL();
  throttle_reported_initiator_origin_.reset();
  onion_location_ = GURL();
  initiator_origin_.reset();
  Observe(new_contents);
}

void OnionLocationTabHelper::SetOnionLocationForTesting(
    const GURL& onion_location,
    std::optional<url::Origin> initiator_origin) {
  onion_location_ = onion_location;
  initiator_origin_ = std::move(initiator_origin);
}

}  // namespace tor
