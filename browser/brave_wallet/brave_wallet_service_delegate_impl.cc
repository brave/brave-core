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

void BraveWalletServiceDelegateImpl::IsExternalWalletInstalled(
    mojom::ExternalWalletType type,
    IsExternalWalletInstalledCallback callback) {
  ExternalWalletsImporter importer(type, context_);
  std::move(callback).Run(importer.IsExternalWalletInstalled());
}

void BraveWalletServiceDelegateImpl::IsExternalWalletInitialized(
    mojom::ExternalWalletType type,
    IsExternalWalletInitializedCallback callback) {
  std::unique_ptr<ExternalWalletsImporter> importer =
      std::make_unique<ExternalWalletsImporter>(type, context_);
  // Do not try to init the importer when external wallet is not installed
  if (!importer->IsExternalWalletInstalled()) {
    std::move(callback).Run(false);
    return;
  }
  importer->Initialize(base::BindOnce(
      &BraveWalletServiceDelegateImpl::ContinueIsExternalWalletInitialized,
      weak_ptr_factory_.GetWeakPtr(), std::move(importer),
      std::move(callback)));
}

void BraveWalletServiceDelegateImpl::ContinueIsExternalWalletInitialized(
    std::unique_ptr<ExternalWalletsImporter> importer,
    IsExternalWalletInitializedCallback callback,
    bool init_success) {
  DCHECK(importer);
  if (init_success) {
    std::move(callback).Run(importer->IsExternalWalletInitialized());
  } else {
    std::move(callback).Run(false);
  }
}

void BraveWalletServiceDelegateImpl::GetImportInfoFromExternalWallet(
    mojom::ExternalWalletType type,
    const std::string& password,
    GetImportInfoCallback callback) {
  std::unique_ptr<ExternalWalletsImporter> importer =
      std::make_unique<ExternalWalletsImporter>(type, context_);
  importer->Initialize(base::BindOnce(
      &BraveWalletServiceDelegateImpl::ContinueGetImportInfoFromExternalWallet,
      weak_ptr_factory_.GetWeakPtr(), std::move(importer), password,
      std::move(callback)));
}

void BraveWalletServiceDelegateImpl::ContinueGetImportInfoFromExternalWallet(
    std::unique_ptr<ExternalWalletsImporter> importer,
    const std::string& password,
    GetImportInfoCallback callback,
    bool init_success) {
  DCHECK(importer);
  if (init_success) {
    DCHECK(importer->IsInitialized());
    importer->GetImportInfo(password, std::move(callback));
  } else {
    std::move(callback).Run(false, ImportInfo(), ImportError::kInternalError);
  }
}

void BraveWalletServiceDelegateImpl::AddEthereumPermission(
    const std::string& origin_spec,
    const std::string& account,
    AddEthereumPermissionCallback callback) {
  bool success =
      permissions::BraveEthereumPermissionContext::AddEthereumPermission(
          context_, origin_spec, account);
  std::move(callback).Run(success);
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
  return contents
             ? contents->GetMainFrame()->GetLastCommittedOrigin().Serialize()
             : "";
}

void BraveWalletServiceDelegateImpl::GetActiveOrigin(
    GetActiveOriginCallback callback) {
  std::move(callback).Run(GetActiveOriginInternal());
}

}  // namespace brave_wallet
