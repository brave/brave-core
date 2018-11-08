/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include <string>
#include <memory>
#include <vector>

#include "bat/ads/ads_client.h"
#include "campaign_info.h"

namespace ads {

class Bundle;
struct CATALOG_STATE;

class Catalog {
 public:
  Catalog(AdsClient* ads_client, Bundle* bundle);
  ~Catalog();

  bool FromJson(const std::string& json);  // Deserialize

  const std::string GetId() const;
  uint64_t GetVersion() const;
  uint64_t GetPing() const;

  const std::vector<CampaignInfo>& GetCampaigns() const;

  void Save(const std::string& json);
  void Reset();

 private:
  bool IsIdValid(const CATALOG_STATE& catalog_state);

  void OnCatalogSaved(const Result result);
  void OnCatalogReset(const Result result);

  AdsClient* ads_client_;  // NOT OWNED
  Bundle* bundle_;  // NOT OWNED

  std::shared_ptr<CATALOG_STATE> catalog_state_;
};

}  // namespace ads
