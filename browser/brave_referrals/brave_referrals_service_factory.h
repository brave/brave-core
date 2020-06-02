/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REFERRALS_BRAVE_REFERRALS_SERVICE_FACTORY_H_
#define BRAVE_BROWSER_BRAVE_REFERRALS_BRAVE_REFERRALS_SERVICE_FACTORY_H_

#include <memory>

#include "base/memory/singleton.h"
#include "brave/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave {
class BraveReferralsService;

class BraveReferralsServiceFactory {
 public:
  static std::unique_ptr<BraveReferralsService> GetForPrefs(
        PrefService* pref_service);
  static BraveReferralsServiceFactory* GetInstance();

 private:
  friend struct base::DefaultSingletonTraits<BraveReferralsServiceFactory>;

  BraveReferralsServiceFactory();
  ~BraveReferralsServiceFactory();

  DISALLOW_COPY_AND_ASSIGN(BraveReferralsServiceFactory);
};

}  // namespace brave

#endif  // BRAVE_BROWSER_BRAVE_REFERRALS_BRAVE_REFERRALS_SERVICE_FACTORY_H_
