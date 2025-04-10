/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/onion_location_tab_helper.h"

#include <memory>

#include "base/memory/ptr_util.h"
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

OnionLocationTabHelper::~OnionLocationTabHelper() = default;

// static
void OnionLocationTabHelper::SetOnionLocation(
    content::WebContents* web_contents,
    const GURL& onion_location) {
  auto* tab_helper = OnionLocationTabHelper::FromWebContents(web_contents);
  if (!tab_helper) {
    return;
  }
  tab_helper->onion_location_ = onion_location;
}

OnionLocationTabHelper::OnionLocationTabHelper(
    content::WebContents* web_contents)
    : content::WebContentsUserData<OnionLocationTabHelper>(*web_contents),
      content::WebContentsObserver(web_contents) {}

WEB_CONTENTS_USER_DATA_KEY_IMPL(OnionLocationTabHelper);

void OnionLocationTabHelper::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted() ||
      navigation_handle->IsSameDocument()) {
    return;
  }
  auto* entry = navigation_handle->GetNavigationEntry();
  if (!entry) {
    return;
  }

  if (navigation_handle->IsServedFromBackForwardCache()) {
    onion_location_ = OnionLocation::Get(entry);
    if (auto* delegate = web_contents()->GetDelegate()) {
      // This forces the page action to update the ui state.
      delegate->NavigationStateChanged(
          web_contents(), content::InvalidateTypes::INVALIDATE_TYPE_URL);
    }
  } else {
    OnionLocation::Set(entry, onion_location_);
  }
}

}  // namespace tor
