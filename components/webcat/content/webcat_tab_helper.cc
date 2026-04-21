/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcat/content/webcat_tab_helper.h"

#include <utility>

#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/page.h"
#include "content/public/browser/web_contents.h"

namespace webcat {

WEB_CONTENTS_USER_DATA_KEY_IMPL(WebcatTabHelper);

WebcatTabHelper::WebcatTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      observation_(this) {
  observation_.Observe(web_contents);
}

WebcatTabHelper::~WebcatTabHelper() = default;

OriginState WebcatTabHelper::GetOriginState(
    const url::Origin& origin) const {
  auto it = origin_states_.find(origin);
  if (it == origin_states_.end()) {
    return OriginState::kUnverified;
  }
  return it->second.state();
}

void WebcatTabHelper::SetOriginState(const url::Origin& origin,
                                     OriginStateData state) {
  origin_states_[origin] = std::move(state);
}

bool WebcatTabHelper::IsOriginVerified(const url::Origin& origin) const {
  auto it = origin_states_.find(origin);
  return it != origin_states_.end() &&
         it->second.state() == OriginState::kVerified;
}

const std::optional<Bundle> WebcatTabHelper::GetManifest(
    const url::Origin& origin) const {
  auto it = origin_states_.find(origin);
  if (it == origin_states_.end() || !it->second.bundle()) {
    return std::nullopt;
  }
  return it->second.bundle();
}

void WebcatTabHelper::MarkOriginVerified(const url::Origin& origin,
                                         Bundle bundle) {
  auto& state = origin_states_[origin];
  state.SetBundle(std::move(bundle));
  state.SetVerified();
  should_show_badge_ = true;
}

void WebcatTabHelper::MarkOriginFailed(const url::Origin& origin,
                                       WebcatError error,
                                       const std::string& detail) {
  auto& state = origin_states_[origin];
  state.SetFailed(error, detail);
  should_show_badge_ = false;
}

void WebcatTabHelper::DidNavigatePrimaryFrame(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle->IsInPrimaryMainFrame() ||
      !navigation_handle->HasCommitted()) {
    return;
  }

  should_show_badge_ = IsOriginVerified(
      url::Origin::Create(navigation_handle->GetURL()));
}

void WebcatTabHelper::PrimaryPageChanged(content::Page& page) {
  should_show_badge_ = IsOriginVerified(
      url::Origin::Create(page.GetMainDocument().GetLastCommittedURL()));
}

void WebcatTabHelper::WebContentsDestroyed() {
  observation_.Reset();
}

}  // namespace webcat