/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_controller.h"
#include "chrome/browser/lifetime/application_lifetime.h"

#include <utility>

#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "brave/common/extensions/extension_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_context.h"

namespace {

  bool ResetCryptoWalletsOnFileTaskRunner(const base::FilePath& path) {
    return base::DeleteFile(path, false);
  }

} // namespace

BraveWalletController::BraveWalletController(content::BrowserContext* context)
    : context_(context),
    file_task_runner_(base::CreateSequencedTaskRunner(
          {base::ThreadPool(), base::MayBlock(),
           base::TaskPriority::BEST_EFFORT,
           base::TaskShutdownBehavior::BLOCK_SHUTDOWN})),
    weak_factory_(this) {
}

BraveWalletController::~BraveWalletController() {
}

void BraveWalletController::ResetCryptoWallets (
    ResetCryptoWalletsCallback callback) {
  Profile* profile = Profile::FromBrowserContext(context_);
  const base::FilePath wallet_data_path = profile->GetPath()
      .AppendASCII("Local Extension Settings")
      .AppendASCII(ethereum_remote_client_extension_id)
      .AppendASCII("000003.LOG");

  base::PostTaskAndReplyWithResult(file_task_runner_.get(), FROM_HERE,
      base::BindOnce(&ResetCryptoWalletsOnFileTaskRunner, wallet_data_path),
      base::BindOnce(&BraveWalletController::OnCryptoWalletsReset,
                     weak_factory_.GetWeakPtr(),
                     std::move(callback))); 
}

void BraveWalletController::OnCryptoWalletsReset(
    ResetCryptoWalletsCallback callback, bool success) {
  std::move(callback).Run(success);
}

void BraveWalletController::RestartBrowser() {
  chrome::AttemptRestart();
}
