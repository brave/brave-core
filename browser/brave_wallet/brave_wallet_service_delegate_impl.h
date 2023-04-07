/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_SERVICE_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_SERVICE_DELEGATE_IMPL_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/scoped_observation.h"
#include "brave/browser/brave_wallet/external_wallets_importer.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/browser/ui/browser_tab_strip_tracker.h"
#include "chrome/browser/ui/browser_tab_strip_tracker_delegate.h"
#include "chrome/browser/ui/tabs/tab_strip_model_observer.h"

namespace content {
class BrowserContext;
}

namespace brave_wallet {

class BraveWalletServiceDelegateImpl : public BraveWalletServiceDelegate,
                                       public TabStripModelObserver,
                                       public BrowserTabStripTrackerDelegate {
 public:
  explicit BraveWalletServiceDelegateImpl(content::BrowserContext* context);
  BraveWalletServiceDelegateImpl(const BraveWalletServiceDelegateImpl&) =
      delete;
  BraveWalletServiceDelegateImpl& operator=(
      const BraveWalletServiceDelegateImpl&) = delete;
  ~BraveWalletServiceDelegateImpl() override;

  void IsExternalWalletInstalled(mojom::ExternalWalletType,
                                 IsExternalWalletInstalledCallback) override;
  void IsExternalWalletInitialized(
      mojom::ExternalWalletType,
      IsExternalWalletInitializedCallback) override;
  void GetImportInfoFromExternalWallet(mojom::ExternalWalletType type,
                                       const std::string& password,
                                       GetImportInfoCallback callback) override;

  void AddPermission(mojom::CoinType coin,
                     const url::Origin& origin,
                     const std::string& account,
                     AddPermissionCallback callback) override;
  void HasPermission(mojom::CoinType coin,
                     const url::Origin& origin,
                     const std::string& account,
                     HasPermissionCallback callback) override;
  void ResetPermission(mojom::CoinType coin,
                       const url::Origin& origin,
                       const std::string& account,
                       ResetPermissionCallback callback) override;
  void IsPermissionDenied(mojom::CoinType coin,
                          const url::Origin& origin,
                          IsPermissionDeniedCallback callback) override;

  void GetActiveOrigin(GetActiveOriginCallback callback) override;

  void ClearWalletUIStoragePartition() override;

  void AddObserver(BraveWalletServiceDelegate::Observer* observer) override;
  void RemoveObserver(BraveWalletServiceDelegate::Observer* observer) override;

  // TabStripModelObserver:
  void OnTabStripModelChanged(
      TabStripModel* tab_strip_model,
      const TabStripModelChange& change,
      const TabStripSelectionChange& selection) override;
  void TabChangedAt(content::WebContents* contents,
                    int index,
                    TabChangeType change_type) override;

  // BrowserTabStripTrackerDelegate:
  bool ShouldTrackBrowser(Browser* browser) override;

 private:
  friend class BraveWalletServiceDelegateImplUnitTest;

  void ContinueIsExternalWalletInitialized(mojom::ExternalWalletType type,
                                           IsExternalWalletInitializedCallback,
                                           bool init_success);
  void ContinueGetImportInfoFromExternalWallet(mojom::ExternalWalletType type,
                                               const std::string& password,
                                               GetImportInfoCallback callback,
                                               bool init_success);

  url::Origin GetActiveOriginInternal();
  void FireActiveOriginChanged();

  raw_ptr<content::BrowserContext> context_ = nullptr;
  base::flat_map<mojom::ExternalWalletType,
                 std::unique_ptr<ExternalWalletsImporter>>
      importers_;

  BrowserTabStripTracker browser_tab_strip_tracker_;
  base::ObserverList<BraveWalletServiceDelegate::Observer> observer_list_;

  base::WeakPtrFactory<BraveWalletServiceDelegateImpl> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_SERVICE_DELEGATE_IMPL_H_
