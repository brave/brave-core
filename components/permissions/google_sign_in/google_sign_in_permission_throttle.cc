// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/permissions/google_sign_in/google_sign_in_permission_throttle.h"

#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "brave/components/permissions/google_sign_in/google_sign_in_permission_util.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/permission_controller_delegate.h"
#include "content/public/browser/web_contents.h"
#include "services/network/public/cpp/resource_request.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"
#include "url/origin.h"

using blink::URLLoaderThrottle;

namespace permissions {

GoogleSignInPermissionThrottle::GoogleSignInPermissionThrottle(
    const content::WebContents::Getter wc_getter,
    scoped_refptr<HostContentSettingsMap> settings_map)
    : wc_getter_(wc_getter), settings_map_(settings_map) {}

void OnPermissionRequestStatus(
    content::NavigationEntry* pending_entry,
    content::WebContents* contents,
    const GURL& request_initiator_url,
    scoped_refptr<HostContentSettingsMap> content_settings,
    URLLoaderThrottle::Delegate* delegate,
    const std::vector<blink::mojom::PermissionStatus>& permission_statuses) {
  DCHECK_EQ(1u, permission_statuses.size());
  // Check if current pending navigation is the one we started out with.
  // This is done to prevent us from accessing a deleted Delegate, if
  // the user navigated away while the prompt was still up, or closed the
  // window
  if (pending_entry != contents->GetController().GetPendingEntry()) {
    return;
  }
  // Now that we have complete the permission request, resume navigation.
  delegate->Resume();
}

std::unique_ptr<blink::URLLoaderThrottle>
GoogleSignInPermissionThrottle::MaybeCreateThrottleFor(
    const network::ResourceRequest& request,
    const content::WebContents::Getter wc_getter,
    HostContentSettingsMap* content_settings) {
  if (!IsGoogleSignInFeatureEnabled()) {
    return nullptr;
  }

  if (request.resource_type !=
      static_cast<int>(blink::mojom::ResourceType::kMainFrame)) {
    return nullptr;
  }

  const auto& request_url = request.url;
  const auto& request_initiator_url =
      GetRequestInitiatingUrlFromRequest(request);

  if (!IsGoogleAuthRelatedRequest(request_url, request_initiator_url)) {
    return nullptr;
  }

  return std::make_unique<GoogleSignInPermissionThrottle>(
      wc_getter,
      base::WrapRefCounted<HostContentSettingsMap>(content_settings));
}

GoogleSignInPermissionThrottle::~GoogleSignInPermissionThrottle() = default;

void GoogleSignInPermissionThrottle::DetachFromCurrentSequence() {}

void HandleRequest(bool* defer,
                   const GURL& request_url,
                   const GURL& request_initiator_url,
                   const content::WebContents::Getter contents_getter,
                   scoped_refptr<HostContentSettingsMap> content_settings,
                   URLLoaderThrottle::Delegate* delegate) {
  auto* contents = contents_getter.Run();
  if (!contents)
    return;

  // Check kGoogleLoginControlType pref and return early if false.
  PrefService* prefs =
      user_prefs::UserPrefs::Get(contents->GetBrowserContext());

  if (!IsGoogleSignInPrefEnabled(prefs)) {
    return;
  }

  GetPermissionAndMaybeCreatePrompt(
      contents, request_initiator_url, defer,
      base::BindOnce(&OnPermissionRequestStatus,
                     contents->GetController().GetPendingEntry(), contents,
                     request_initiator_url, content_settings, delegate));
}

void GoogleSignInPermissionThrottle::WillStartRequest(
    network::ResourceRequest* request,
    bool* defer) {
  const auto& request_url = request->url;
  const auto& request_initiator_url =
      GetRequestInitiatingUrlFromRequest(*request);

  HandleRequest(defer, request_url, request_initiator_url, wc_getter_,
                settings_map_, delegate_);
}

}  // namespace permissions
