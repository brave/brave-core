// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_BRAVE_SEARCH_DEFAULT_HOST_H_
#define BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_BRAVE_SEARCH_DEFAULT_HOST_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_search/common/brave_search_default.mojom.h"

class TemplateURLService;
class TemplateURL;
class PrefService;
class PrefRegistrySimple;

namespace brave_search {

class BraveSearchDefaultHost final
    : public brave_search::mojom::BraveSearchDefault {
 public:
  static void RegisterProfilePrefs(PrefRegistrySimple* registry);

  BraveSearchDefaultHost(const BraveSearchDefaultHost&) = delete;
  BraveSearchDefaultHost& operator=(const BraveSearchDefaultHost&) = delete;

  BraveSearchDefaultHost(const std::string& host,
                         TemplateURLService* template_url_service,
                         PrefService* prefs);
  ~BraveSearchDefaultHost() override;

  // brave_search::mojom::BraveSearchDefault:
  void GetCanSetDefaultSearchProvider(
      GetCanSetDefaultSearchProviderCallback callback) override;
  void SetIsDefaultSearchProvider() override;

 private:
  bool CanSetDefaultSearchProvider(TemplateURL* provider, bool is_historic);
  uint64_t GetMaxDailyCanAskCount();
  uint64_t GetMaxTotalCanAskCount();

  bool can_set_default_ = false;
  const std::string host_;
  raw_ptr<TemplateURLService> template_url_service_ = nullptr;
  raw_ptr<PrefService> prefs_ = nullptr;
};

}  // namespace brave_search

#endif  // BRAVE_COMPONENTS_BRAVE_SEARCH_BROWSER_BRAVE_SEARCH_DEFAULT_HOST_H_
