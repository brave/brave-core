// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/google_sign_in_permission/google_sign_in_permission_util.h"

#include <utility>
#include <vector>

#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/google_sign_in_permission/features.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/permission_controller_delegate.h"
#include "content/public/browser/permission_descriptor_util.h"
#include "content/public/browser/permission_request_description.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"
#include "services/network/public/cpp/resource_request.h"
#include "third_party/blink/public/common/permissions/permission_utils.h"
#include "third_party/blink/public/mojom/permissions/permission_status.mojom-shared.h"

namespace google_sign_in_permission {

namespace {

constexpr char kGoogleAuthPattern[] =
    "https://accounts.google.com/o/oauth2/auth/*";
constexpr char kFirebasePattern[] = "https://[*.]firebaseapp.com/__/auth/*";

bool IsFirebaseAuthUrl(const GURL& gurl) {
  return GetFirebaseAuthPattern().Matches(gurl);
}

bool IsGoogleAuthUrl(const GURL& gurl) {
  return GetGoogleAuthPattern().Matches(gurl);
}

bool RequestMatchesAuthPatterns(const GURL& gurl) {
  return IsFirebaseAuthUrl(gurl) || IsGoogleAuthUrl(gurl);
}

}  // namespace

const ContentSettingsPattern& GetGoogleAuthPattern() {
  static const base::NoDestructor<ContentSettingsPattern> google_pattern(
      ContentSettingsPattern::FromString(kGoogleAuthPattern));
  return *google_pattern;
}

const ContentSettingsPattern& GetFirebaseAuthPattern() {
  static const base::NoDestructor<ContentSettingsPattern> firebase_pattern(
      ContentSettingsPattern::FromString(kFirebasePattern));
  return *firebase_pattern;
}

// Heuristics to determine if the auth flow uses 3P cookies
bool AuthFlowUses3PCookies(const GURL& request_url) {
  if (IsGoogleAuthUrl(request_url)) {
    // Check `redirect_uri` param in request_url for string "storagerelay"
    // Ref: manual inspection
    return request_url.has_query() &&
           request_url.query_piece().find("redirect_uri=storagerelay") !=
               std::string::npos;
  }
  if (IsFirebaseAuthUrl(request_url)) {
    // Check if redirect sign-in via authType=signInViaRedirect param
    // Ref:
    // https://firebase.google.com/docs/auth/web/redirect-best-practices
    return request_url.has_query() &&
           request_url.query_piece().find("authType=signInViaRedirect") !=
               std::string::npos;
  }
  return false;
}

bool IsGoogleAuthRelatedRequest(const GURL& request_url,
                                const GURL& request_initiator_url) {
  static const base::NoDestructor<GURL> kGoogleAuthUrl([] {
    DCHECK(!GetGoogleAuthPattern().HasDomainWildcard());
    DCHECK(!GetGoogleAuthPattern().GetHost().empty());
    return GURL("https://" + GetGoogleAuthPattern().GetHost());
  }());
  return request_url.SchemeIsHTTPOrHTTPS() &&
         request_initiator_url.SchemeIsHTTPOrHTTPS() &&
         RequestMatchesAuthPatterns(request_url) &&
         !RequestMatchesAuthPatterns(request_initiator_url) &&
         !net::registry_controlled_domains::SameDomainOrHost(
             request_initiator_url, *kGoogleAuthUrl,
             net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES) &&
         AuthFlowUses3PCookies(request_url);
}

bool IsGoogleSignInFeatureEnabled() {
  return base::FeatureList::IsEnabled(features::kBraveGoogleSignInPermission);
}

blink::mojom::PermissionStatus GetCurrentGoogleSignInPermissionStatus(
    content::PermissionControllerDelegate* permission_controller,
    content::WebContents* contents,
    const GURL& request_initiator_url) {
  return permission_controller->GetPermissionStatusForCurrentDocument(
      blink::PermissionType::BRAVE_GOOGLE_SIGN_IN,
      contents->GetPrimaryMainFrame(), /*should_include_device_status=*/false);
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
      rfh,
      content::PermissionRequestDescription(
          content::PermissionDescriptorUtil::
              CreatePermissionDescriptorForPermissionType(
                  blink::PermissionType::BRAVE_GOOGLE_SIGN_IN),
          /*user_gesture*/ true),
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

// Reloads the top-level tab after the user has made a decision on the
// permission prompt. Only used for popups, and only if the user has granted the
// permission.
void ReloadTab(
    base::WeakPtr<content::WebContents> contents,
    const std::vector<blink::mojom::PermissionStatus>& permission_statuses) {
  DCHECK_EQ(1u, permission_statuses.size());
  if (contents &&
      permission_statuses[0] == blink::mojom::PermissionStatus::GRANTED) {
    contents->GetController().Reload(content::ReloadType::NORMAL, true);
  }
}

bool CanCreateWindow(content::RenderFrameHost* opener,
                     const GURL& opener_url,
                     const GURL& target_url) {
  content::WebContents* contents =
      content::WebContents::FromRenderFrameHost(opener);

  if (IsGoogleSignInFeatureEnabled() &&
      IsGoogleAuthRelatedRequest(target_url, opener_url)) {
    return GetPermissionAndMaybeCreatePrompt(
        contents, opener_url, nullptr,
        base::BindOnce(&ReloadTab, contents->GetWeakPtr()));
  }
  // If not applying Google Sign-In permission logic, open window
  return true;
}
}  // namespace google_sign_in_permission
