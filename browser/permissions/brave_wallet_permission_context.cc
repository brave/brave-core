/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/permissions/brave_wallet_permission_context.h"

#include <algorithm>
#include <optional>
#include <utility>
#include <variant>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/functional/bind.h"
#include "brave/browser/brave_wallet/brave_wallet_tab_helper.h"
#include "brave/browser/permissions/brave_wallet_account_chooser_contexts.h"
#include "brave/browser/permissions/brave_wallet_account_chooser_contexts_factory.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/permissions/contexts/brave_wallet_account_chooser_context.h"
#include "brave/components/permissions/permission_lifetime_utils.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/content_settings/core/common/content_settings.h"
#include "components/content_settings/core/common/content_settings_types.h"
#include "components/permissions/features.h"
#include "components/permissions/permission_request.h"
#include "components/permissions/permission_request_manager.h"
#include "components/permissions/permissions_client.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/global_routing_id.h"
#include "content/public/browser/permission_controller_delegate.h"
#include "content/public/browser/permission_descriptor_util.h"
#include "content/public/browser/permission_request_description.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "services/network/public/mojom/permissions_policy/permissions_policy_feature.mojom.h"
#include "url/origin.h"

namespace permissions {

namespace {

constexpr brave_wallet::mojom::CoinType kPermissionedCoins[] = {
    brave_wallet::mojom::CoinType::ETH, brave_wallet::mojom::CoinType::SOL,
    brave_wallet::mojom::CoinType::ADA};

std::optional<brave_wallet::mojom::CoinType> CoinTypeForPermission(
    blink::PermissionType permission) {
  switch (permission) {
    case blink::PermissionType::BRAVE_ETHEREUM:
      return brave_wallet::mojom::CoinType::ETH;
    case blink::PermissionType::BRAVE_SOLANA:
      return brave_wallet::mojom::CoinType::SOL;
    case blink::PermissionType::BRAVE_CARDANO:
      return brave_wallet::mojom::CoinType::ADA;
    default:
      return std::nullopt;
  }
}

std::optional<brave_wallet::mojom::CoinType> CoinTypeForContentSettingsType(
    ContentSettingsType type) {
  switch (type) {
    case ContentSettingsType::BRAVE_ETHEREUM:
      return brave_wallet::mojom::CoinType::ETH;
    case ContentSettingsType::BRAVE_SOLANA:
      return brave_wallet::mojom::CoinType::SOL;
    case ContentSettingsType::BRAVE_CARDANO:
      return brave_wallet::mojom::CoinType::ADA;
    default:
      return std::nullopt;
  }
}

std::optional<brave_wallet::mojom::CoinType> CoinTypeForRequestType(
    RequestType type) {
  switch (type) {
    case RequestType::kBraveEthereum:
      return brave_wallet::mojom::CoinType::ETH;
    case RequestType::kBraveSolana:
      return brave_wallet::mojom::CoinType::SOL;
    case RequestType::kBraveCardano:
      return brave_wallet::mojom::CoinType::ADA;
    default:
      return std::nullopt;
  }
}

BraveWalletAccountChooserContext* GetChooserContext(
    content::BrowserContext* context,
    brave_wallet::mojom::CoinType coin) {
  if (!context) {
    return nullptr;
  }
  auto* contexts =
      brave_wallet::BraveWalletAccountChooserContextsFactory::GetForContext(
          context);
  return contexts ? contexts->Get(coin) : nullptr;
}

BraveWalletAccountChooserContext* GetChooserContextForPermission(
    content::BrowserContext* context,
    blink::PermissionType permission) {
  const auto coin = CoinTypeForPermission(permission);
  return coin ? GetChooserContext(context, *coin) : nullptr;
}

// Maps mojom::PermissionLifetimeOption onto the lifetimes
// CreatePermissionLifetimeOptions offers, which the enum indexes positionally.
std::optional<base::TimeDelta> LifetimeForOption(
    brave_wallet::mojom::PermissionLifetimeOption option) {
  // Without the feature there is no lifetime concept at all, so grants are
  // permanent rather than refused.
  if (!base::FeatureList::IsEnabled(features::kPermissionLifetime)) {
    return std::nullopt;
  }
  const auto options = CreatePermissionLifetimeOptions();
  const auto index = static_cast<size_t>(option);
  if (index >= options.size()) {
    return std::nullopt;
  }
  return options[index].lifetime;
}

// The user's account choices were written to the chooser context before the
// request was resolved, so the granted set is read back from there rather than
// inferred from the request outcome.
void OnWalletPermissionRequestResolved(
    blink::PermissionType permission,
    content::GlobalRenderFrameHostId rfh_id,
    std::vector<std::string> addresses,
    BraveWalletPermissionContext::RequestWalletPermissionsCallback callback,
    const std::vector<content::PermissionResult>&) {
  auto* rfh = content::RenderFrameHost::FromID(rfh_id);
  if (auto* web_contents = content::WebContents::FromRenderFrameHost(rfh)) {
    if (auto* tab_helper =
            brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents)) {
      tab_helper->ClearPendingConnectAccounts();
    }
  }

  auto allowed = BraveWalletPermissionContext::GetAllowedAccounts(
      permission, rfh, addresses);
  std::move(callback).Run(allowed.value_or(std::vector<std::string>()));
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

void BraveWalletPermissionContext::UpdateSetting(
    const PermissionRequestData& request_data,
    const PermissionSetting& setting,
    bool is_one_time) {
  // Grants belong to the chooser context and are written by AcceptOrCancel.
  // The guard registered for this context has no Allow state, so recording one
  // here would store a value its registration rejects.
  if (std::holds_alternative<ContentSetting>(setting) &&
      std::get<ContentSetting>(setting) == CONTENT_SETTING_ALLOW) {
    return;
  }
  ContentSettingPermissionContextBase::UpdateSetting(request_data, setting,
                                                     is_one_time);
}

// static
void BraveWalletPermissionContext::RequestWalletPermissions(
    const std::vector<std::string>& addresses,
    blink::PermissionType permission,
    const url::Origin& requesting_origin,
    content::RenderFrameHost* rfh,
    RequestWalletPermissionsCallback callback) {
  if (!rfh || addresses.empty()) {
    std::move(callback).Run({});
    return;
  }

  auto* web_contents = content::WebContents::FromRenderFrameHost(rfh);
  if (!web_contents) {
    std::move(callback).Run({});
    return;
  }

  if (requesting_origin != rfh->GetLastCommittedOrigin()) {
    std::move(callback).Run({});
    return;
  }

  content::PermissionControllerDelegate* delegate =
      web_contents->GetBrowserContext()->GetPermissionControllerDelegate();
  if (!delegate) {
    std::move(callback).Run({});
    return;
  }

  auto* chooser = GetChooserContextForPermission(
      web_contents->GetBrowserContext(), permission);
  if (!chooser) {
    std::move(callback).Run({});
    return;
  }

  // A blocked origin must not be prompted.
  if (!chooser->CanRequestObjectPermission(requesting_origin)) {
    std::move(callback).Run({});
    return;
  }

  // Nothing new to ask for. The guard stays Ask even while accounts are
  // granted, so PermissionContextBase would otherwise prompt again for an
  // already fully connected site.
  const bool all_granted =
      std::ranges::all_of(addresses, [&](const std::string& address) {
        return chooser->HasAccountPermission(requesting_origin, address);
      });
  if (all_granted) {
    std::move(callback).Run(addresses);
    return;
  }

  // The panel reads these back out when it builds its URL; the request itself
  // only names the origin.
  if (auto* tab_helper =
          brave_wallet::BraveWalletTabHelper::FromWebContents(web_contents)) {
    tab_helper->SetPendingConnectAccounts(addresses);
  }

  content::PermissionRequestDescription desc(
      content::PermissionDescriptorUtil::
          CreatePermissionDescriptorForPermissionTypes({permission}),
      rfh->HasTransientUserActivation(), requesting_origin.GetURL());

  // This gives high priority to request and avoids reordering.
  desc.embedded_permission_request_descriptor =
      blink::mojom::EmbeddedPermissionRequestDescriptor::New();

  delegate->RequestPermissionsFromCurrentDocument(
      rfh, desc,
      base::BindOnce(&OnWalletPermissionRequestResolved, permission,
                     rfh->GetGlobalId(), addresses, std::move(callback)));
}

// static
void BraveWalletPermissionContext::AcceptOrCancel(
    const std::vector<std::string>& accounts,
    brave_wallet::mojom::PermissionLifetimeOption option,
    content::WebContents* web_contents) {
  PermissionRequestManager* manager =
      PermissionRequestManager::FromWebContents(web_contents);
  if (!manager || manager->Requests().empty()) {
    return;
  }

  // Write the grants before resolving, so the reply callback reads them.
  if (!accounts.empty()) {
    PermissionRequest* request = manager->Requests().front().get();
    const auto coin = CoinTypeForRequestType(request->request_type());
    auto* chooser =
        coin ? GetChooserContext(web_contents->GetBrowserContext(), *coin)
             : nullptr;
    if (chooser) {
      const auto origin = url::Origin::Create(request->requesting_origin());
      const auto lifetime = LifetimeForOption(option);
      for (const auto& account : accounts) {
        chooser->GrantAccountPermission(origin, account, lifetime);
      }
    }
  }

  std::vector<PermissionRequest*> allowed_requests;
  std::vector<PermissionRequest*> cancelled_requests;
  for (const auto& request : manager->Requests()) {
    if (accounts.empty()) {
      cancelled_requests.push_back(request.get());
    } else {
      allowed_requests.push_back(request.get());
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
  manager->Dismiss(/*prompt_options=*/std::monostate());
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
  if (!web_contents ||
      web_contents->GetPrimaryMainFrame()->GetLastCommittedURL().is_empty()) {
    return std::vector<std::string>();
  }

  auto* chooser = GetChooserContextForPermission(
      web_contents->GetBrowserContext(), permission);
  if (!chooser) {
    return std::nullopt;
  }

  const auto origin = url::Origin::Create(rfh->GetLastCommittedURL());
  std::vector<std::string> allowed_accounts;
  for (const auto& address : addresses) {
    if (chooser->HasAccountPermission(origin, address)) {
      allowed_accounts.push_back(address);
    }
  }

  return allowed_accounts;
}

// static
bool BraveWalletPermissionContext::IsPermissionDenied(
    blink::PermissionType permission,
    content::BrowserContext* context,
    const url::Origin& origin) {
  auto* chooser = GetChooserContextForPermission(context, permission);
  return chooser && !chooser->CanRequestObjectPermission(origin);
}

// static
bool BraveWalletPermissionContext::AddPermission(
    blink::PermissionType permission,
    content::BrowserContext* context,
    const url::Origin& origin,
    const std::string& account) {
  auto* chooser = GetChooserContextForPermission(context, permission);
  if (!chooser || account.empty()) {
    return false;
  }

  chooser->GrantAccountPermission(origin, account, /*lifetime=*/std::nullopt);
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

  auto* chooser = GetChooserContextForPermission(context, permission);
  if (!chooser) {
    return false;
  }

  // A blocked guard hides every grant, so this reports false there too.
  *has_permission = chooser->HasAccountPermission(origin, account);
  return true;
}

// static
bool BraveWalletPermissionContext::ResetPermission(
    blink::PermissionType permission,
    content::BrowserContext* context,
    const url::Origin& origin,
    const std::string& account) {
  auto* chooser = GetChooserContextForPermission(context, permission);
  if (!chooser) {
    return false;
  }

  chooser->RevokeAccountPermission(origin, account);
  return true;
}

// static
std::vector<std::string>
BraveWalletPermissionContext::GetWebSitesWithPermission(
    blink::PermissionType permission,
    content::BrowserContext* context) {
  auto* chooser = GetChooserContextForPermission(context, permission);
  if (!chooser) {
    return {};
  }

  std::vector<std::string> result;
  for (const auto& origin : chooser->GetOriginsWithGrantedAccounts()) {
    result.push_back(origin.Serialize());
  }
  return result;
}

// static
bool BraveWalletPermissionContext::ResetWebSitePermission(
    blink::PermissionType permission,
    content::BrowserContext* context,
    const std::string& formed_website) {
  const GURL url(formed_website);
  if (!url.is_valid()) {
    return false;
  }

  auto* chooser = GetChooserContextForPermission(context, permission);
  if (!chooser) {
    return false;
  }

  chooser->RevokeAllAccountPermissionsForOrigin(url::Origin::Create(url));
  return true;
}

// static
void BraveWalletPermissionContext::ResetPermissionsForAccount(
    content::BrowserContext* context,
    ContentSettingsType content_settings_type,
    const std::string& account) {
  const auto coin = CoinTypeForContentSettingsType(content_settings_type);
  if (!coin || account.empty()) {
    return;
  }

  if (auto* chooser = GetChooserContext(context, *coin)) {
    chooser->RevokeAccountPermissionForAllOrigins(account);
  }
}

// static
void BraveWalletPermissionContext::ResetAllPermissions(
    content::BrowserContext* context) {
  HostContentSettingsMap* map =
      PermissionsClient::Get()->GetSettingsMap(context);

  for (const auto coin : kPermissionedCoins) {
    if (auto* chooser = GetChooserContext(context, coin)) {
      chooser->RevokeAllAccountPermissions();
    }
    // Also drop any site-level block, matching the previous behaviour of
    // clearing the whole content settings type.
    if (map) {
      if (const auto guard =
              BraveWalletAccountChooserContext::GuardTypeForCoin(coin)) {
        map->ClearSettingsForOneType(*guard);
      }
    }
  }
}

}  // namespace permissions
