/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_MANAGED_PERCENTAGE_INTERMEDIATE_H_
#define BRAVE_COMPONENTS_P3A_MANAGED_PERCENTAGE_INTERMEDIATE_H_

#include <memory>

#include "base/containers/flat_set.h"
#include "base/json/json_value_converter.h"
#include "base/values.h"
#include "brave/components/p3a/managed/remote_metric_intermediate.h"

namespace p3a {

class PercentageIntermediateDefinition {
 public:
  PercentageIntermediateDefinition();
  ~PercentageIntermediateDefinition();

  PercentageIntermediateDefinition(const PercentageIntermediateDefinition&) =
      delete;
  PercentageIntermediateDefinition& operator=(
      const PercentageIntermediateDefinition&) = delete;
  PercentageIntermediateDefinition(PercentageIntermediateDefinition&&);

  static void RegisterJSONConverter(
      base::JSONValueConverter<PercentageIntermediateDefinition>* converter);

  base::Value numerator;
  base::Value denominator;
  int multiplier = 1;  // Optional multiplier, defaults to 1
};

// Intermediate that computes (numerator/denominator) * 100 * multiplier from
// two source intermediates.
class PercentageIntermediate : public RemoteMetricIntermediate {
 public:
  PercentageIntermediate(PercentageIntermediateDefinition definition,
                         Delegate* delegate);
  ~PercentageIntermediate() override;

  PercentageIntermediate(const PercentageIntermediate&) = delete;
  PercentageIntermediate& operator=(const PercentageIntermediate&) = delete;

  bool Init() override;
  base::Value Process() override;
  base::flat_set<std::string_view> GetStorageKeys() const override;
  void OnLastUsedProfilePrefsChanged(PrefService* profile_prefs) override;

 private:
  PercentageIntermediateDefinition definition_;
  std::unique_ptr<RemoteMetricIntermediate> numerator_intermediate_;
  std::unique_ptr<RemoteMetricIntermediate> denominator_intermediate_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_MANAGED_PERCENTAGE_INTERMEDIATE_H_
