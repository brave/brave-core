/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ETHEREUM_REMOTE_CLIENT_ETHEREUM_REMOTE_CLIENT_SERVICE_H_
#define BRAVE_BROWSER_ETHEREUM_REMOTE_CLIENT_ETHEREUM_REMOTE_CLIENT_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

class EthereumRemoteClientDelegate;
class PrefService;

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

class EthereumRemoteClientService final : public KeyedService {
 public:
  explicit EthereumRemoteClientService(
      content::BrowserContext* context,
      std::unique_ptr<EthereumRemoteClientDelegate>
          ethereum_remote_client_delegate);
  EthereumRemoteClientService(const EthereumRemoteClientService&) = delete;
  EthereumRemoteClientService& operator=(const EthereumRemoteClientService&) =
      delete;
  ~EthereumRemoteClientService() override;
  using LoadUICallback = base::OnceCallback<void()>;

  void ResetCryptoWallets();
  std::string GetWalletSeed(std::vector<uint8_t> key);
  bool IsLegacyCryptoWalletsSetup() const;
  bool IsCryptoWalletsReady() const;
  void MaybeLoadCryptoWalletsExtension(LoadUICallback callback);
  void CryptoWalletsExtensionReady();
  void UnloadCryptoWalletsExtension();

  static std::string GetEthereumRemoteClientSeedFromRootSeed(
      const std::string& seed);
  static bool SealSeed(const std::string& seed,
                       const std::string& key,
                       const std::string& nonce,
                       std::string* cipher_seed);
  static bool OpenSeed(const std::string& cipher_seed,
                       const std::string& key,
                       const std::string& nonce,
                       std::string* seed);
  static void SaveToPrefs(PrefService* prefs,
                          const std::string& cipher_seed,
                          const std::string& nonce);
  static bool LoadFromPrefs(PrefService* prefs,
                            std::string* cipher_seed,
                            std::string* nonce);
  static std::string GetRandomNonce();
  static std::string GetRandomSeed();
  static constexpr size_t kNonceByteLength = 12;
  static constexpr size_t kSeedByteLength = 32;

 private:
  bool LoadRootSeedInfo(std::vector<uint8_t> key, std::string* seed);

  raw_ptr<content::BrowserContext> context_ = nullptr;
  std::unique_ptr<EthereumRemoteClientDelegate>
      ethereum_remote_client_delegate_;
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  LoadUICallback load_ui_callback_;
};

#endif  // BRAVE_BROWSER_ETHEREUM_REMOTE_CLIENT_ETHEREUM_REMOTE_CLIENT_SERVICE_H_
