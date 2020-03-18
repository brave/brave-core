/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"

#include <string>
#include <utility>

#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_controller.h"
#include "brave/components/brave_wallet/browser/browser_wallet_delegate.h"
#include "content/public/browser/browser_context.h"

BraveWalletService::BraveWalletService(content::BrowserContext* context,
      std::unique_ptr<BrowserWalletDelegate> browser_wallet_delegate)
    : context_(context),
      controller_(new BraveWalletController(context,
          std::move(browser_wallet_delegate))),
      weak_factory_(this) {
}

BraveWalletService::~BraveWalletService() {}
