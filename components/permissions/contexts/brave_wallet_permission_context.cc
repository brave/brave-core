/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"

#include <optional>
#include <utility>

#include "base/barrier_callback.h"
#include "base/check.h"
#include "base/strings/string_util.h"
#include "base/types/zip.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/permissions/permission_lifetime_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/permission_manager.h"
#include "components/permissions/permission_request.h"
#include "components/permissions/permission_request_data.h"
#include "components/permissions/permission_request_id.h"
#include "components/permissions/permission_request_manager.h"
#include "components/permissions/permissions_client.h"
#include "components/permissions/resolvers/content_setting_permission_resolver.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/permission_controller_delegate.h"
#include "content/public/browser/permission_descriptor_util.h"
#include "content/public/browser/permission_request_description.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "services/network/public/mojom/permissions_policy/permissions_policy_feature.mojom.h"
#include "url/origin.h"

namespace permissions {

namespace {

bool IsAccepted(PermissionRequest* request,
                const std::vector<std::string>& accounts) {
  for (const auto& account : accounts) {
    if (base::EndsWith(request->requesting_origin().host(), account,
                       base::CompareCase::INSENSITIVE_ASCII)) {
      return true;
    }
  }

  return false;
}

std::optional<std::string> HandleWalletPermissionResult(
    const std::string& address,
    const std::vector<content::PermissionResult>& results) {
  if (results.size() == 1 &&
      results.front().status == blink::mojom::PermissionStatus::GRANTED) {
    return address;
  }
  return std::nullopt;
}

void AggregatePermissionResults(
    BraveWalletPermissionContext::RequestWalletPermissionsCallback callback,
    std::vector<std::optional<std::string>> results) {
  std::vector<std::string> allowed_accounts;
  for (auto& result : results) {
    if (result) {
      allowed_accounts.push_back(std::move(*result));
    }
  }
  std::move(callback).Run(std::move(allowed_accounts));
}

}  // namespace

BraveWalletPermissionContext::BraveWalletPermissionContext(
    content::BrowserContext* browser_context,
    ContentSettingsType content_settings_type)
    : ContentSettingPermissionContextBase(
          browser_context,
          content_settings_type,
          network::mojom::PermissionsPolicyFeature::kNotFound) {}

BraveWalletPermissionContext::~BraveWalletPermissionContext() = default;

bool BraveWalletPermissionContext::IsRestrictedToSecureOrigins() const {
  // For parity with Crypto Wallets and MM we should allow a permission prompt
  // to be shown for HTTP sites. Developers often use localhost for development
  // for example.
  return false;
}

// static
void BraveWalletPermissionContext::AcceptOrCancel(
    const std::vector<std::string>& accounts,
    brave_wallet::mojom::PermissionLifetimeOption option,
    content::WebContents* web_contents) {
  PermissionRequestManager* manager =
      PermissionRequestManager::FromWebContents(web_contents);
  if (!manager) {
    return;
  }

  std::vector<PermissionRequest*> allowed_requests;
  std::vector<PermissionRequest*> cancelled_requests;
  for (const auto& request : manager->Requests()) {
    if (IsAccepted(request.get(), accounts)) {
      const auto options = CreatePermissionLifetimeOptions();
      SetRequestLifetime(options, static_cast<size_t>(option), request.get());
      allowed_requests.push_back(request.get());
    } else {
      cancelled_requests.push_back(request.get());
    }
  }

  manager->AcceptDenyCancel(allowed_requests, std::vector<PermissionRequest*>(),
                            cancelled_requests);
}

// static
void BraveWalletPermissionContext::Cancel(content::WebContents* web_contents) {
  PermissionRequestManager* manager =
      PermissionRequestManager::FromWebContents(web_contents);
  if (!manager) {
    return;
  }

  // Dismiss all requests.
  manager->Dismiss();
}

// static
bool BraveWalletPermissionContext::HasRequestsInProgress(
    content::RenderFrameHost* rfh,
    permissions::RequestType request_type) {
  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  PermissionRequestManager* manager =
      PermissionRequestManager::FromWebContents(web_contents);
  if (!manager) {
    return false;
  }

  // Only check the first entry because it will not be grouped with other types
  return !manager->Requests().empty() &&
         manager->Requests()[0]->request_type() == request_type;
}

// static
void BraveWalletPermissionContext::RequestWalletPermissions(
    const std::vector<std::string>& addresses,
    blink::PermissionType permission,
    content::RenderFrameHost* rfh,
    RequestWalletPermissionsCallback callback) {
  if (!rfh) {
    std::move(callback).Run({});
    return;
  }

  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  if (!web_contents) {
    std::move(callback).Run({});
    return;
  }

  content::PermissionControllerDelegate* delegate =
      web_contents->GetBrowserContext()->GetPermissionControllerDelegate();
  if (!delegate) {
    std::move(callback).Run({});
    return;
  }

  RequestType request_type = ContentSettingsTypeToRequestType(
      PermissionUtil::PermissionTypeToContentSettingsTypeSafe(permission));

  std::vector<url::Origin> addresses_with_origin;
  for (auto& address : addresses) {
    if (auto origin_with_address = brave_wallet::GetSubRequestOrigin(
            request_type, rfh->GetLastCommittedOrigin(), address)) {
      addresses_with_origin.push_back(std::move(*origin_with_address));
    }
  }
  if (addresses.size() != addresses_with_origin.size()) {
    std::move(callback).Run({});
    return;
  }

  auto barrier_callback = base::BarrierCallback<std::optional<std::string>>(
      addresses_with_origin.size(),
      base::BindOnce(&AggregatePermissionResults, std::move(callback)));

  for (auto [address, origin_with_address] :
       base::zip(addresses, addresses_with_origin)) {
    content::PermissionRequestDescription desc(
        content::PermissionDescriptorUtil::
            CreatePermissionDescriptorForPermissionTypes({permission}),
        rfh->HasTransientUserActivation(), origin_with_address.GetURL());

    // This gives high priority to request and avoids reordering.
    desc.embedded_permission_request_descriptor =
        blink::mojom::EmbeddedPermissionRequestDescriptor::New();

    delegate->RequestPermissionsFromCurrentDocument(
        rfh, desc,
        base::BindOnce(&HandleWalletPermissionResult, address)
            .Then(barrier_callback));
  }
}

// static
std::optional<std::vector<std::string>>
BraveWalletPermissionContext::GetAllowedAccounts(
    blink::PermissionType permission,
    content::RenderFrameHost* rfh,
    const std::vector<std::string>& addresses) {
  if (!rfh) {
    return std::nullopt;
  }

  // Fail if there is no last committed URL yet
  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  if (web_contents->GetPrimaryMainFrame()->GetLastCommittedURL().is_empty()) {
    return std::vector<std::string>();
  }

  content::PermissionControllerDelegate* delegate =
      web_contents->GetBrowserContext()->GetPermissionControllerDelegate();
  if (!delegate) {
    return std::nullopt;
  }

  const ContentSettingsType content_settings_type =
      PermissionUtil::PermissionTypeToContentSettingsTypeSafe(permission);

  std::vector<std::string> allowed_accounts;
  url::Origin origin = url::Origin::Create(rfh->GetLastCommittedURL());
  for (const auto& address : addresses) {
    auto sub_request_origin = brave_wallet::GetSubRequestOrigin(
        ContentSettingsTypeToRequestType(content_settings_type), origin,
        address);
    if (sub_request_origin) {
      // `GetPermissionResultForEmbeddedRequester` allows us to specify
      // `requesting_origin` instead of obtaining it from `rfh`.
      auto status =
          delegate
              ->GetPermissionResultForEmbeddedRequester(
                  content::PermissionDescriptorUtil::
                      CreatePermissionDescriptorForPermissionType(permission),
                  rfh, *sub_request_origin)
              .status;

      if (status == blink::mojom::PermissionStatus::GRANTED) {
        allowed_accounts.push_back(address);
      }
    }
  }

  return allowed_accounts;
}

// static
bool BraveWalletPermissionContext::IsPermissionDenied(
    blink::PermissionType permission,
    content::BrowserContext* context,
    const url::Origin& origin) {
  content::PermissionControllerDelegate* delegate =
      context->GetPermissionControllerDelegate();
  if (!delegate) {
    return false;
  }

  return delegate->GetPermissionStatus(
             content::PermissionDescriptorUtil::
                 CreatePermissionDescriptorForPermissionType(permission),
             origin.GetURL(),
             origin.GetURL()) == blink::mojom::PermissionStatus::DENIED;
}

// static
bool BraveWalletPermissionContext::AddPermission(
    blink::PermissionType permission,
    content::BrowserContext* context,
    const url::Origin& origin,
    const std::string& account) {
  bool has_permission;
  if (!HasPermission(permission, context, origin, account, &has_permission)) {
    return false;
  }

  if (has_permission) {
    return true;
  }

  const ContentSettingsType content_settings_type =
      PermissionUtil::PermissionTypeToContentSettingsTypeSafe(permission);

  auto origin_wallet_address = brave_wallet::GetSubRequestOrigin(
      ContentSettingsTypeToRequestType(content_settings_type), origin, account);
  if (!origin_wallet_address) {
    return false;
  }

  PermissionsClient::Get()
      ->GetSettingsMap(context)
      ->SetContentSettingDefaultScope(
          origin_wallet_address->GetURL(), origin_wallet_address->GetURL(),
          content_settings_type, ContentSetting::CONTENT_SETTING_ALLOW);

  return true;
}

// static
bool BraveWalletPermissionContext::HasPermission(
    blink::PermissionType permission,
    content::BrowserContext* context,
    const url::Origin& origin,
    const std::string& account,
    bool* has_permission) {
  CHECK(has_permission);
  *has_permission = false;
  content::PermissionControllerDelegate* delegate =
      context->GetPermissionControllerDelegate();
  if (!delegate) {
    return false;
  }

  if (IsPermissionDenied(permission, context, origin)) {
    return true;
  }

  const ContentSettingsType content_settings_type =
      PermissionUtil::PermissionTypeToContentSettingsTypeSafe(permission);

  auto origin_wallet_address = brave_wallet::GetSubRequestOrigin(
      ContentSettingsTypeToRequestType(content_settings_type), origin, account);
  if (!origin_wallet_address) {
    return false;
  }

  auto status = delegate->GetPermissionStatus(
      content::PermissionDescriptorUtil::
          CreatePermissionDescriptorForPermissionType(permission),
      origin_wallet_address->GetURL(), origin_wallet_address->GetURL());

  *has_permission = status == blink::mojom::PermissionStatus::GRANTED;
  return true;
}

// static
bool BraveWalletPermissionContext::ResetPermission(
    blink::PermissionType permission,
    content::BrowserContext* context,
    const url::Origin& origin,
    const std::string& account) {
  content::PermissionControllerDelegate* delegate =
      context->GetPermissionControllerDelegate();
  if (!delegate) {
    return false;
  }

  const ContentSettingsType content_settings_type =
      PermissionUtil::PermissionTypeToContentSettingsTypeSafe(permission);

  auto origin_wallet_address = brave_wallet::GetSubRequestOrigin(
      ContentSettingsTypeToRequestType(content_settings_type), origin, account);
  if (!origin_wallet_address) {
    return false;
  }

  delegate->ResetPermission(permission, origin_wallet_address->GetURL(),
                            origin_wallet_address->GetURL());
  return true;
}

// static
std::vector<std::string>
BraveWalletPermissionContext::GetWebSitesWithPermission(
    blink::PermissionType permission,
    content::BrowserContext* context) {
  const ContentSettingsType content_settings_type =
      PermissionUtil::PermissionTypeToContentSettingsTypeSafe(permission);

  HostContentSettingsMap* map =
      PermissionsClient::Get()->GetSettingsMap(context);
  ContentSettingsForOneType settings =
      map->GetSettingsForOneType(content_settings_type);

  std::vector<std::string> result;
  for (const auto& setting : settings) {
    if (setting.GetContentSetting() != CONTENT_SETTING_ALLOW) {
      continue;
    }
    result.push_back(setting.primary_pattern.ToString());
  }

  return result;
}

// static
bool BraveWalletPermissionContext::ResetWebSitePermission(
    blink::PermissionType permission,
    content::BrowserContext* context,
    const std::string& formed_website) {
  content::PermissionControllerDelegate* delegate =
      context->GetPermissionControllerDelegate();
  GURL url(formed_website);
  if (!delegate || !url.is_valid()) {
    return false;
  }

  delegate->ResetPermission(permission, url, url);
  return true;
}

void BraveWalletPermissionContext::ResetAllPermissions(
    content::BrowserContext* context) {
  HostContentSettingsMap* map =
      PermissionsClient::Get()->GetSettingsMap(context);
  map->ClearSettingsForOneType(ContentSettingsType::BRAVE_ETHEREUM);
  map->ClearSettingsForOneType(ContentSettingsType::BRAVE_SOLANA);
  map->ClearSettingsForOneType(ContentSettingsType::BRAVE_CARDANO);
}

}  // namespace permissions
