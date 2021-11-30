/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_wallet/brave_wallet_service_delegate_impl.h"

#include <utility>

#include "brave/components/permissions/contexts/brave_ethereum_permission_context.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace brave_wallet {

namespace {

content::WebContents* GetActiveWebContents() {
  Browser* browser = chrome::FindLastActive();
  return browser ? browser->tab_strip_model()->GetActiveWebContents() : nullptr;
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

void BraveWalletServiceDelegateImpl::IsCryptoWalletsInstalled(
    IsCryptoWalletsInstalledCallback callback) {
  ExternalWalletsImporter importer(
      ExternalWalletsImporter::WalletType::kCryptoWallets, context_);
  std::move(callback).Run(importer.IsExternalWalletInstalled());
}

void BraveWalletServiceDelegateImpl::IsMetaMaskInstalled(
    IsMetaMaskInstalledCallback callback) {
  ExternalWalletsImporter importer(
      ExternalWalletsImporter::WalletType::kMetaMask, context_);
  std::move(callback).Run(importer.IsExternalWalletInstalled());
}

void BraveWalletServiceDelegateImpl::GetImportInfoFromCryptoWallets(
    const std::string& password,
    GetImportInfoCallback callback) {
  importer_.reset(new ExternalWalletsImporter(
      ExternalWalletsImporter::WalletType::kCryptoWallets, context_));
  importer_->Initialize(base::BindOnce(
      &BraveWalletServiceDelegateImpl::ContinueImportInfo,
      weak_ptr_factory_.GetWeakPtr(), password, std::move(callback)));
}

void BraveWalletServiceDelegateImpl::GetImportInfoFromMetaMask(
    const std::string& password,
    GetImportInfoCallback callback) {
  importer_.reset(new ExternalWalletsImporter(
      ExternalWalletsImporter::WalletType::kMetaMask, context_));
  importer_->Initialize(base::BindOnce(
      &BraveWalletServiceDelegateImpl::ContinueImportInfo,
      weak_ptr_factory_.GetWeakPtr(), password, std::move(callback)));
}

void BraveWalletServiceDelegateImpl::ContinueImportInfo(
    const std::string& password,
    GetImportInfoCallback callback,
    bool init_success) {
  if (init_success) {
    DCHECK(importer_->IsInitialized());
    importer_->GetImportInfo(password, std::move(callback));
  } else {
    importer_.reset();
    std::move(callback).Run(false, ImportInfo(), ImportError::kInternalError);
  }
}

void BraveWalletServiceDelegateImpl::HasEthereumPermission(
    const std::string& origin_spec,
    const std::string& account,
    HasEthereumPermissionCallback callback) {
  bool has_permission = false;
  bool success =
      permissions::BraveEthereumPermissionContext::HasEthereumPermission(
          context_, origin_spec, account, &has_permission);
  std::move(callback).Run(success, has_permission);
}

void BraveWalletServiceDelegateImpl::ResetEthereumPermission(
    const std::string& origin_spec,
    const std::string& account,
    ResetEthereumPermissionCallback callback) {
  bool success =
      permissions::BraveEthereumPermissionContext::ResetEthereumPermission(
          context_, origin_spec, account);
  std::move(callback).Run(success);
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
  if (!contents || contents != GetActiveWebContents())
    return;

  FireActiveOriginChanged();
}

void BraveWalletServiceDelegateImpl::FireActiveOriginChanged() {
  for (auto& observer : observer_list_)
    observer.OnActiveOriginChanged(GetActiveOriginInternal());
}

std::string BraveWalletServiceDelegateImpl::GetActiveOriginInternal() {
  content::WebContents* contents = GetActiveWebContents();
  return contents ? contents->GetMainFrame()
                        ->GetLastCommittedURL()
                        .GetOrigin()
                        .spec()
                  : "";
}

void BraveWalletServiceDelegateImpl::GetActiveOrigin(
    GetActiveOriginCallback callback) {
  std::move(callback).Run(GetActiveOriginInternal());
}

}  // namespace brave_wallet
