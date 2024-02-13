/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_SERVICE_H_
#define BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/json/json_value_converter.h"
#include "base/memory/weak_ptr.h"
#include "base/timer/wall_clock_timer.h"
#include "base/values.h"
#include "brave/components/request_otr/browser/request_otr_component_installer.h"
#include "brave/components/request_otr/browser/request_otr_rule.h"
#include "brave/components/request_otr/browser/request_otr_service.h"
#include "components/keyed_service/core/keyed_service.h"

class GURL;
class PrefRegistrySimple;
class PrefService;

namespace request_otr {

// Manage Request-OTR-tab ruleset and provide an API for navigation throttles to
// call to determine if a URL is included in the ruleset.
class RequestOTRService : public KeyedService,
                          public RequestOTRComponentInstallerPolicy::Observer {
 public:
  enum class RequestOTRActionOption {
    kAsk = 0,
    kAlways,
    kNever,
  };

  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  explicit RequestOTRService(PrefService* profile_prefs);
  RequestOTRService(const RequestOTRService&) = delete;
  RequestOTRService& operator=(const RequestOTRService&) = delete;
  ~RequestOTRService() override;
  void OnRulesReady(const std::string&) override;

  bool ShouldBlock(const GURL& url) const;

 private:
  void UpdateP3AMetrics();

  std::vector<std::unique_ptr<RequestOTRRule>> rules_;
  base::flat_set<std::string> host_cache_;

  raw_ptr<PrefService> profile_prefs_;
  base::WallClockTimer p3a_timer_;

  base::WeakPtrFactory<RequestOTRService> weak_factory_{this};
};

}  // namespace request_otr

#endif  // BRAVE_COMPONENTS_REQUEST_OTR_BROWSER_REQUEST_OTR_SERVICE_H_
