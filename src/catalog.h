/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_CATALOG_H_
#define BAT_ADS_CATALOG_H_

#include <string>
#include <memory>
#include <vector>

#include "bat/ads/ads_client.h"
#include "campaign_info.h"

namespace ads {

class Bundle;
struct CatalogState;

class Catalog {
 public:
  Catalog(AdsClient* ads_client, Bundle* bundle);
  ~Catalog();

  bool FromJson(const std::string& json);  // Deserialize

  const std::string GetId() const;
  uint64_t GetVersion() const;
  uint64_t GetPing() const;

  const std::vector<CampaignInfo>& GetCampaigns() const;

  const std::vector<IssuerInfo>& GetIssuers() const;

  void Save(const std::string& json, OnSaveCallback callback);
  void Reset(OnSaveCallback callback);

 private:
  bool IsIdValid(const CatalogState& catalog_state);

  AdsClient* ads_client_;  // NOT OWNED
  Bundle* bundle_;  // NOT OWNED

  std::shared_ptr<CatalogState> catalog_state_;
};

}  // namespace ads

#endif  // BAT_ADS_CATALOG_H_
