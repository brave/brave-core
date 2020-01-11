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

  using ResetCryptoWalletsCallback = base::OnceCallback<void(bool)>;

  void ResetCryptoWallets(
    ResetCryptoWalletsCallback callback);
  void RestartBrowser();

 private:
  content::BrowserContext* context_;
  scoped_refptr<base::SequencedTaskRunner> file_task_runner_;
  base::WeakPtrFactory<BraveWalletController> weak_factory_;

  void OnCryptoWalletsReset(
    ResetCryptoWalletsCallback callback, bool success);

  DISALLOW_COPY_AND_ASSIGN(BraveWalletController);
};

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_CONTROLLER_H_
