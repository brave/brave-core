/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/binance/browser/binance_service.h"

#include <string>
#include <utility>

#include "base/task/post_task.h"
#include "base/task_runner_util.h"
#include "brave/components/binance/browser/binance_controller.h"
#include "content/public/browser/browser_context.h"

BinanceService::BinanceService(content::BrowserContext* context)
    : context_(context),
      controller_(new BinanceController(context)),
      weak_factory_(this) {
}

BinanceService::~BinanceService() {}
