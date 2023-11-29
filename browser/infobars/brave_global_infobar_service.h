/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_INFOBARS_BRAVE_GLOBAL_INFOBAR_SERVICE_H_
#define BRAVE_BROWSER_INFOBARS_BRAVE_GLOBAL_INFOBAR_SERVICE_H_

#include <map>
#include <memory>

#include "components/infobars/core/infobar_delegate.h"
#include "components/keyed_service/core/keyed_service.h"

class BraveGlobalInfoBarManager;
class PrefService;

class BraveGlobalInfobarService : public KeyedService {
 public:
  explicit BraveGlobalInfobarService(PrefService* prefs);
  ~BraveGlobalInfobarService() override;

  BraveGlobalInfobarService(const BraveGlobalInfobarService&) = delete;
  BraveGlobalInfobarService& operator=(const BraveGlobalInfobarService&) =
      delete;

  void ShowAlwaysStartInfobar();

 private:
  std::map<infobars::InfoBarDelegate::InfoBarIdentifier,
           std::unique_ptr<BraveGlobalInfoBarManager>>
      infobar_managers_;
  raw_ptr<PrefService> prefs_ = nullptr;
};

#endif  // BRAVE_BROWSER_INFOBARS_BRAVE_GLOBAL_INFOBAR_SERVICE_H_
