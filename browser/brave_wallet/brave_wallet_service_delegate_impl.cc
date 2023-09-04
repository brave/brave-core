/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_service_delegate_impl.h"

#include <utility>

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/components/permissions/contexts/brave_wallet_permission_context.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"
#include "url/origin.h"

using content::StoragePartition;

namespace brave_wallet {

namespace {

content::WebContents* g_web_contents_for_testing = nullptr;

content::WebContents* GetActiveWebContents() {
  if (g_web_contents_for_testing) {
    return g_web_contents_for_testing;
  }

  Browser* browser = chrome::FindLastActive();
  return browser ? browser->tab_strip_model()->GetActiveWebContents() : nullptr;
}

void ClearWalletStoragePartition(content::BrowserContext* context,
                                 const GURL& url) {
  CHECK(context);
  auto* partition = context->GetDefaultStoragePartition();
  partition->ClearDataForOrigin(
      StoragePartition::REMOVE_DATA_MASK_ALL,
      StoragePartition::QUOTA_MANAGED_STORAGE_MASK_ALL, url, base::DoNothing());
}

}  // namespace

BraveWalletServiceDelegateImpl::BraveWalletServiceDelegateImpl(
    content::BrowserContext* context)
    : context_(context),
      browser_tab_strip_tracker_(this, this),
      weak_ptr_factory_(this) {
  browser_tab_strip_tracker_.Init();
}

BraveWalletServiceDelegateImpl::~BraveWalletServiceDelegateImpl() = default;

// static
void BraveWalletServiceDelegateImpl::SetActiveWebContentsForTesting(
    content::WebContents* web_contents) {
  g_web_contents_for_testing = web_contents;
}

void BraveWalletServiceDelegateImpl::AddObserver(
    BraveWalletServiceDelegate::Observer* observer) {
  observer_list_.AddObserver(observer);
}

void BraveWalletServiceDelegateImpl::RemoveObserver(
    BraveWalletServiceDelegate::Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

bool BraveWalletServiceDelegateImpl::ShouldTrackBrowser(Browser* browser) {
  return browser->profile() == Profile::FromBrowserContext(context_);
}

void BraveWalletServiceDelegateImpl::IsExternalWalletInstalled(
    mojom::ExternalWalletType type,
    IsExternalWalletInstalledCallback callback) {
  ExternalWalletsImporter importer(type, context_);
  std::move(callback).Run(importer.IsExternalWalletInstalled());
}

void BraveWalletServiceDelegateImpl::IsExternalWalletInitialized(
    mojom::ExternalWalletType type,
    IsExternalWalletInitializedCallback callback) {
  importers_[type] = std::make_unique<ExternalWalletsImporter>(type, context_);
  // Do not try to init the importer when external wallet is not installed
  if (!importers_[type]->IsExternalWalletInstalled()) {
    std::move(callback).Run(false);
    return;
  }
  if (importers_[type]->IsInitialized()) {
    ContinueIsExternalWalletInitialized(type, std::move(callback), true);
  } else {
    importers_[type]->Initialize(base::BindOnce(
        &BraveWalletServiceDelegateImpl::ContinueIsExternalWalletInitialized,
        weak_ptr_factory_.GetWeakPtr(), type, std::move(callback)));
  }
}

void BraveWalletServiceDelegateImpl::ContinueIsExternalWalletInitialized(
    mojom::ExternalWalletType type,
    IsExternalWalletInitializedCallback callback,
    bool init_success) {
  DCHECK(importers_[type]);
  if (init_success) {
    std::move(callback).Run(importers_[type]->IsExternalWalletInitialized());
  } else {
    std::move(callback).Run(false);
  }
}

void BraveWalletServiceDelegateImpl::GetImportInfoFromExternalWallet(
    mojom::ExternalWalletType type,
    const std::string& password,
    GetImportInfoCallback callback) {
  if (!importers_[type]) {
    importers_[type] =
        std::make_unique<ExternalWalletsImporter>(type, context_);
  }
  if (importers_[type]->IsInitialized()) {
    ContinueGetImportInfoFromExternalWallet(type, password, std::move(callback),
                                            true);
  } else {
    importers_[type]->Initialize(base::BindOnce(
        &BraveWalletServiceDelegateImpl::
            ContinueGetImportInfoFromExternalWallet,
        weak_ptr_factory_.GetWeakPtr(), type, password, std::move(callback)));
  }
}

void BraveWalletServiceDelegateImpl::ContinueGetImportInfoFromExternalWallet(
    mojom::ExternalWalletType type,
    const std::string& password,
    GetImportInfoCallback callback,
    bool init_success) {
  DCHECK(importers_[type]);
  if (init_success) {
    DCHECK(importers_[type]->IsInitialized());
    importers_[type]->GetImportInfo(password, std::move(callback));
  } else {
    std::move(callback).Run(false, ImportInfo(), ImportError::kInternalError);
  }
}

bool BraveWalletServiceDelegateImpl::AddPermission(mojom::CoinType coin,
                                                   const url::Origin& origin,
                                                   const std::string& account) {
  auto type = CoinTypeToPermissionType(coin);
  if (!type) {
    return false;
  }

  return permissions::BraveWalletPermissionContext::AddPermission(
      *type, context_, origin, account);
}

bool BraveWalletServiceDelegateImpl::HasPermission(mojom::CoinType coin,
                                                   const url::Origin& origin,
                                                   const std::string& account) {
  bool has_permission = false;
  auto type = CoinTypeToPermissionType(coin);
  if (!type) {
    return false;
  }

  bool success = permissions::BraveWalletPermissionContext::HasPermission(
      *type, context_, origin, account, &has_permission);
  return success && has_permission;
}

bool BraveWalletServiceDelegateImpl::ResetPermission(
    mojom::CoinType coin,
    const url::Origin& origin,
    const std::string& account) {
  auto type = CoinTypeToPermissionType(coin);
  if (!type) {
    return false;
  }

  return permissions::BraveWalletPermissionContext::ResetPermission(
      *type, context_, origin, account);
}

bool BraveWalletServiceDelegateImpl::IsPermissionDenied(
    mojom::CoinType coin,
    const url::Origin& origin) {
  auto type = CoinTypeToPermissionType(coin);
  if (!type) {
    return false;
  }

  return permissions::BraveWalletPermissionContext::IsPermissionDenied(
      *type, context_, origin);
}

void BraveWalletServiceDelegateImpl::OnTabStripModelChanged(
    TabStripModel* tab_strip_model,
    const TabStripModelChange& change,
    const TabStripSelectionChange& selection) {
  FireActiveOriginChanged();
}

void BraveWalletServiceDelegateImpl::TabChangedAt(
    content::WebContents* contents,
    int index,
    TabChangeType change_type) {
  if (!contents || contents != GetActiveWebContents()) {
    return;
  }

  FireActiveOriginChanged();
}

void BraveWalletServiceDelegateImpl::FireActiveOriginChanged() {
  mojom::OriginInfoPtr origin_info =
      MakeOriginInfo(GetActiveOriginInternal().value_or(url::Origin()));
  for (auto& observer : observer_list_) {
    observer.OnActiveOriginChanged(origin_info);
  }
}

absl::optional<url::Origin>
BraveWalletServiceDelegateImpl::GetActiveOriginInternal() {
  content::WebContents* contents = GetActiveWebContents();
  return contents ? contents->GetPrimaryMainFrame()->GetLastCommittedOrigin()
                  : absl::optional<url::Origin>();
}

absl::optional<url::Origin> BraveWalletServiceDelegateImpl::GetActiveOrigin() {
  return GetActiveOriginInternal();
}

void BraveWalletServiceDelegateImpl::ClearWalletUIStoragePartition() {
  ClearWalletStoragePartition(context_, GURL(kBraveUIWalletURL));
  ClearWalletStoragePartition(context_, GURL(kBraveUIWalletPanelURL));
}

}  // namespace brave_wallet
