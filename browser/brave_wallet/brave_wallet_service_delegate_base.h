/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_SERVICE_DELEGATE_BASE_H_
#define BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_SERVICE_DELEGATE_BASE_H_

#include <string>

#include "base/auto_reset.h"
#include "base/files/file_path.h"
#include "base/memory/raw_ptr.h"
#include "base/observer_list.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/content_settings/core/browser/content_settings_observer.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"

namespace content {
class BrowserContext;
}

namespace url {
class Origin;
}

namespace brave_wallet {

// Shared BraveWalletServiceDelegate implementation between Desktop and Android.
class BraveWalletServiceDelegateBase : public BraveWalletServiceDelegate,
                                       public content_settings::Observer {
 public:
  explicit BraveWalletServiceDelegateBase(content::BrowserContext* context);
  BraveWalletServiceDelegateBase(const BraveWalletServiceDelegateBase&) =
      delete;
  BraveWalletServiceDelegateBase& operator=(
      const BraveWalletServiceDelegateBase&) = delete;
  ~BraveWalletServiceDelegateBase() override;

  // Delegates created in scope of returned object will have wallet autolock
  // disabled.
  static base::AutoReset<bool> GetScopedDisableAutolockForTesting();

  bool HasPermission(mojom::CoinType coin,
                     const url::Origin& origin,
                     const std::string& account) override;
  bool ResetPermission(mojom::CoinType coin,
                       const url::Origin& origin,
                       const std::string& account) override;
  void ResetPermissionsForAccount(mojom::CoinType coin,
                                  const std::string& account) override;
  bool IsPermissionDenied(mojom::CoinType coin,
                          const url::Origin& origin) override;
  void ResetAllPermissions() override;

  base::FilePath GetWalletBaseDirectory() override;

  bool IsPrivateWindow() override;

  bool IsAutolockEnabled() override;

  void AddObserver(BraveWalletServiceDelegate::Observer* observer) override;
  void RemoveObserver(BraveWalletServiceDelegate::Observer* observer) override;

  // content_settings::Observer:
  void OnContentSettingChanged(const ContentSettingsPattern& primary_pattern,
                               const ContentSettingsPattern& secondary_pattern,
                               ContentSettingsType content_type) override;

 protected:
  base::FilePath wallet_base_directory_;
  bool is_private_window_ = false;
  raw_ptr<content::BrowserContext> context_ = nullptr;
  base::ObserverList<BraveWalletServiceDelegate::Observer> observer_list_;

 private:
  base::ScopedObservation<HostContentSettingsMap, content_settings::Observer>
      content_settings_observation_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_BROWSER_BRAVE_WALLET_BRAVE_WALLET_SERVICE_DELEGATE_BASE_H_
