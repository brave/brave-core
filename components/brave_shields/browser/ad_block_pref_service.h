/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_PREF_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_PREF_SERVICE_H_

#include <memory>
#include <string>

#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/browser_thread.h"

class PrefChangeRegistrar;
class PrefService;

namespace brave_shields {

class AdBlockService;

class AdBlockPrefService : public KeyedService {
 public:
  explicit AdBlockPrefService(AdBlockService* ad_block_service,
                              PrefService* prefs);
  ~AdBlockPrefService() override;

 private:
  void OnPreferenceChanged(const std::string& pref_name);

  AdBlockService* ad_block_service_;  // not owned
  PrefService* prefs_;                // not owned
  std::unique_ptr<PrefChangeRegistrar, content::BrowserThread::DeleteOnUIThread>
      pref_change_registrar_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_PREF_SERVICE_H_
