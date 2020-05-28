/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/containers/queue.h"
#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/scoped_observer.h"
#include "base/values.h"
#include "components/keyed_service/core/keyed_service.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/extension_registry_observer.h"
#include "url/gurl.h"

class BraveWalletDelegate;
class PrefChangeRegistrar;
class PrefService;

namespace base {
class FilePath;
class SequencedTaskRunner;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

class BraveWalletService : public KeyedService,
    public extensions::ExtensionRegistryObserver {
 public:
  explicit BraveWalletService(content::BrowserContext* context,
      std::unique_ptr<BraveWalletDelegate> brave_wallet_delegate);
  ~BraveWalletService() override;
  using LoadUICallback = base::OnceCallback<void()>;

  void ResetCryptoWallets();
  std::string GetWalletSeed(std::vector<uint8_t> key);
  std::string GetBitGoSeed(std::vector<uint8_t> key);
  bool IsCryptoWalletsSetup() const;
  bool IsCryptoWalletsReady() const;
  bool ShouldShowLazyLoadInfobar() const;
  void LoadCryptoWalletsExtension(LoadUICallback callback);
  void CryptoWalletsExtensionReady();

  static std::string GetEthereumRemoteClientSeedFromRootSeed(
      const std::string& seed);
  static std::string GetBitGoSeedFromRootSeed(
      const std::string& seed);
  static bool SealSeed(const std::string& seed, const std::string& key,
      const std::string& nonce, std::string* cipher_seed);
  static bool OpenSeed(const std::string& cipher_seed,
      const std::string& key, const std::string& nonce, std::string* seed);
  static void SaveToPrefs(PrefService* prefs, const std::string& cipher_seed,
      const std::string& nonce);
  static bool LoadFromPrefs(PrefService* prefs, std::string* cipher_seed,
      std::string* nonce);
  static std::string GetRandomNonce();
  static std::string GetRandomSeed();
  static const size_t kNonceByteLength;
  static const size_t kSeedByteLength;

 private:
  bool LoadRootSeedInfo(std::vector<uint8_t> key, std::string* seed);
  void RemoveUnusedWeb3ProviderContentScripts();
  void OnPreferenceChanged();
  // ExtensionRegistryObserver implementation.
  void OnExtensionLoaded(content::BrowserContext* browser_context,
                         const extensions::Extension* extension) override;

  content::BrowserContext* context_;
  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;
  std::unique_ptr<BraveWalletDelegate> brave_wallet_delegate_;
  ScopedObserver<extensions::ExtensionRegistry,
      extensions::ExtensionRegistryObserver> extension_registry_observer_{this};
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  LoadUICallback load_ui_callback_;
  base::WeakPtrFactory<BraveWalletService> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(BraveWalletService);
};

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_
