/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BINANCE_BROWSER_BINANCE_SERVICE_H_
#define BRAVE_COMPONENTS_BINANCE_BROWSER_BINANCE_SERVICE_H_

#include <memory>

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_observer.h"
#include "components/keyed_service/core/keyed_service.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

class BinanceController;
class Profile;

class BinanceService : public KeyedService {
 public:
  explicit BinanceService(content::BrowserContext* context);
  ~BinanceService() override;

  bool Init();
  BinanceController* controller() const { return controller_.get(); }

 private:
  content::BrowserContext* context_;
  std::unique_ptr<BinanceController> controller_;
  base::WeakPtrFactory<BinanceService> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(BinanceService);
};

#endif  // BRAVE_COMPONENTS_BINANCE_BROWSER_BINANCE_SERVICE_H_
