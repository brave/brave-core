/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_SERVICE_H_
#define BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_SERVICE_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

class PrefRegistrySimple;
class PrefService;

namespace de_amp {

class DeAmpService : public KeyedService {
 public:
  explicit DeAmpService(PrefService* prefs);
  ~DeAmpService() override;
  DeAmpService(const DeAmpService&) = delete;
  DeAmpService& operator=(const DeAmpService&) = delete;

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  void ToggleDeAmp(const bool on = true);
  void DisableDeAmpForTest();
  bool IsEnabled();
  static bool FindCanonicalLinkIfAMP(const std::string& body,
                                     std::string* canonical_link);
  static bool VerifyCanonicalLink(const GURL canonical_link,
                                  const GURL original_url);

 private:
  raw_ptr<PrefService> prefs_ = nullptr;
};

}  // namespace de_amp

#endif  // BRAVE_COMPONENTS_DE_AMP_BROWSER_DE_AMP_SERVICE_H_
