// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/permissions/google_sign_in/google_sign_in_permission_util.h"

#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/no_destructor.h"
#include "brave/components/constants/pref_names.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/permissions/features.h"
#include "components/permissions/permissions_client.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "services/network/public/cpp/resource_request.h"
#include "third_party/blink/public/mojom/permissions/permission_status.mojom-shared.h"

namespace permissions {

namespace {

bool IsGoogleAuthUrl(const GURL& gurl) {
  // Check if pattern matches the URL.
  return GetGoogleAuthPattern().Matches(gurl) ||
         GetFirebaseAuthPattern().Matches(gurl);
}

}  // namespace

ContentSettingsPattern GetGoogleAuthPattern() {
  static const base::NoDestructor<ContentSettingsPattern> google_pattern(
      ContentSettingsPattern::FromString(permissions::kGoogleAuthPattern));
  return *google_pattern;
}

ContentSettingsPattern GetFirebaseAuthPattern() {
  static const base::NoDestructor<ContentSettingsPattern> firebase_pattern(
      ContentSettingsPattern::FromString(permissions::kFirebasePattern));
  return *firebase_pattern;
}

bool IsGoogleAuthRelatedRequest(const GURL& request_url,
                                const GURL& request_initiator_url) {
  return request_url.SchemeIsHTTPOrHTTPS() &&
         request_initiator_url.SchemeIsHTTPOrHTTPS() &&
         IsGoogleAuthUrl(request_url) &&
         !IsGoogleAuthUrl(request_initiator_url) &&
         !net::registry_controlled_domains::SameDomainOrHost(
             request_initiator_url, GURL(GetGoogleAuthPattern().GetHost()),
             net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

bool IsGoogleSignInFeatureEnabled() {
  return base::FeatureList::IsEnabled(features::kBraveGoogleSignInPermission);
}

bool IsGoogleSignInPrefEnabled(PrefService* prefs) {
  return prefs->FindPreference(kGoogleLoginControlType) &&
         prefs->GetBoolean(kGoogleLoginControlType);
}

blink::mojom::PermissionStatus GetCurrentGoogleSignInPermissionStatus(
    content::PermissionControllerDelegate* permission_controller,
    content::WebContents* contents,
    const GURL& request_initiator_url) {
  return permission_controller->GetPermissionStatusForCurrentDocument(
      blink::PermissionType::BRAVE_GOOGLE_SIGN_IN,
      contents->GetPrimaryMainFrame());
}

void CreateGoogleSignInPermissionRequest(
    bool* defer,
    content::PermissionControllerDelegate* permission_controller,
    content::RenderFrameHost* rfh,
    const GURL& request_initiator_url,
    base::OnceCallback<void(const std::vector<blink::mojom::PermissionStatus>&)>
        callback) {
  if (!rfh->IsDocumentOnLoadCompletedInMainFrame()) {
    return;
  }
  if (defer) {
    *defer = true;
  }
  permission_controller->RequestPermissionsFromCurrentDocument(
      {blink::PermissionType::BRAVE_GOOGLE_SIGN_IN}, rfh, true,
      std::move(callback));
}

bool GetPermissionAndMaybeCreatePrompt(
    content::WebContents* contents,
    const GURL& request_initiator_url,
    bool* defer,
    base::OnceCallback<void(const std::vector<blink::mojom::PermissionStatus>&)>
        permission_result_callback) {
  // Check current permission status
  content::PermissionControllerDelegate* permission_controller =
      contents->GetBrowserContext()->GetPermissionControllerDelegate();
  auto current_status = GetCurrentGoogleSignInPermissionStatus(
      permission_controller, contents, request_initiator_url);

  switch (current_status) {
    case blink::mojom::PermissionStatus::GRANTED: {
      return true;
    }

    case blink::mojom::PermissionStatus::DENIED: {
      return false;
    }

    case blink::mojom::PermissionStatus::ASK: {
      CreateGoogleSignInPermissionRequest(
          defer, permission_controller, contents->GetPrimaryMainFrame(),
          request_initiator_url, std::move(permission_result_callback));
      return false;
    }
  }
}

GURL GetRequestInitiatingUrlFromRequest(
    const network::ResourceRequest& request) {
  if (!request.request_initiator) {
    return request.referrer;
  }
  return request.request_initiator->GetURL();
}

bool CanCreateWindow(content::RenderFrameHost* opener,
                     const GURL& opener_url,
                     const GURL& target_url) {
  content::WebContents* contents =
      content::WebContents::FromRenderFrameHost(opener);

  if (IsGoogleSignInFeatureEnabled() &&
      IsGoogleAuthRelatedRequest(target_url, opener_url)) {
    if (!IsGoogleSignInPrefEnabled(
            user_prefs::UserPrefs::Get(contents->GetBrowserContext()))) {
      return false;
    }

    return GetPermissionAndMaybeCreatePrompt(contents, opener_url, nullptr,
                                             base::DoNothing());
  }
  // If not applying Google Sign-In permission logic, open window
  return true;
}
}  // namespace permissions
