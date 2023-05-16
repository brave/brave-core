/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/fl/predictors/variables/shopping_intensity_predictor_variable.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"

#include "brave/components/brave_ads/core/internal/common/logging_util.h"

#include <map>

namespace brave_ads {

ShoppingIntensityPredictorVariable::ShoppingIntensityPredictorVariable(
    brave_federated::mojom::CovariateType predictor_type)
    : predictor_type_(predictor_type) {}

brave_federated::mojom::DataType
ShoppingIntensityPredictorVariable::GetDataType() const {
  return brave_federated::mojom::DataType::kInt;
}

brave_federated::mojom::CovariateType
ShoppingIntensityPredictorVariable::GetType() const {
  return predictor_type_;
}

std::string ShoppingIntensityPredictorVariable::GetValue() const {

  AdsClientHelper::GetInstance()->GetBrowsingHistory(
      50, 3, base::BindOnce([](const std::vector<GURL> &urls) {
        int shopping_events = 0;
        for (const auto &url : urls) {
          BLOG(2, "url");
          BLOG(2, url);
          BLOG(2, url.path());
          if (url.path().find("cart") != std::string::npos) {
            BLOG(2, "cart found");
            shopping_events += 1;
          }
          BLOG(2, "shopping events:");
          BLOG(2, shopping_events);
        }
      }));

  return base::NumberToString(0);
}

} // namespace brave_ads
