// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/google_sign_in_permission/google_sign_in_permission_throttle.h"

#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "brave/components/google_sign_in_permission/google_sign_in_permission_util.h"
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

namespace google_sign_in_permission {

namespace {

void OnPermissionRequestStatus(
    content::NavigationEntry* pending_entry,
    content::WebContents::Getter wc_getter,
    const GURL& request_initiator_url,
    URLLoaderThrottle::Delegate* delegate,
    const std::vector<blink::mojom::PermissionStatus>& permission_statuses) {
  DCHECK_EQ(1u, permission_statuses.size());

  auto* contents = wc_getter.Run();
  if (!contents || !pending_entry) {
    return;
  }

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

}  // namespace

GoogleSignInPermissionThrottle::GoogleSignInPermissionThrottle(
    const content::WebContents::Getter& wc_getter)
    : wc_getter_(wc_getter) {}

std::unique_ptr<blink::URLLoaderThrottle>
GoogleSignInPermissionThrottle::MaybeCreateThrottleFor(
    const network::ResourceRequest& request,
    const content::WebContents::Getter& wc_getter) {
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

  return std::make_unique<GoogleSignInPermissionThrottle>(wc_getter);
}

GoogleSignInPermissionThrottle::~GoogleSignInPermissionThrottle() = default;

void GoogleSignInPermissionThrottle::DetachFromCurrentSequence() {}

void GoogleSignInPermissionThrottle::WillStartRequest(
    network::ResourceRequest* request,
    bool* defer) {
  const auto& request_initiator_url =
      GetRequestInitiatingUrlFromRequest(*request);

  auto* contents = wc_getter_.Run();
  if (!contents) {
    return;
  }

  GetPermissionAndMaybeCreatePrompt(
      contents, request_initiator_url, defer,
      base::BindOnce(&OnPermissionRequestStatus,
                     contents->GetController().GetPendingEntry(), wc_getter_,
                     request_initiator_url, delegate_));
}

}  // namespace google_sign_in_permission
