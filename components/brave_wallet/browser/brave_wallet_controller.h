/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_CONTROLLER_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback_forward.h"
#include "base/containers/queue.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/values.h"
#include "url/gurl.h"
#include "chrome/browser/profiles/profile.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

class BraveWalletController {
 public:
  explicit BraveWalletController(content::BrowserContext* context);
  ~BraveWalletController();

  void ResetCryptoWallets();
  void CloseTabsAndRestart();
  std::string GetWalletSeed(
      std::vector<uint8_t> key);

  static std::string GetEthereumRemoteClientSeedFromRootSeed(
      const std::string& seed);
  static bool SealSeed(const std::string& seed, const std::string& key,
      const std::string& nonce, std::string* cipher_seed);
  static bool OpenSeed(const std::string& cipher_seed,
      const std::string& key, const std::string& nonce, std::string* seed);
  static void SaveToPrefs(Profile* profile, const std::string& cipher_seed,
      const std::string& nonce);
  static bool LoadFromPrefs(Profile* profile, std::string* cipher_seed,
      std::string* nonce);
  static std::string GetRandomNonce();
  static std::string GetRandomSeed();
  static const size_t kNonceByteLength;
  static const size_t kSeedByteLength;

 private:
  content::BrowserContext* context_;
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  base::WeakPtrFactory<BraveWalletController> weak_factory_;

  void OnCryptoWalletsReset(bool success);

  DISALLOW_COPY_AND_ASSIGN(BraveWalletController);
};

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_CONTROLLER_H_
