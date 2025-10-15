/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_MANAGED_BUCKET_INTERMEDIATE_H_
#define BRAVE_COMPONENTS_P3A_MANAGED_BUCKET_INTERMEDIATE_H_

#include <memory>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/json/json_value_converter.h"
#include "base/values.h"
#include "brave/components/p3a/managed/remote_metric_intermediate.h"

namespace p3a {

class BucketIntermediateDefinition {
 public:
  BucketIntermediateDefinition();
  ~BucketIntermediateDefinition();

  BucketIntermediateDefinition(const BucketIntermediateDefinition&) = delete;
  BucketIntermediateDefinition& operator=(const BucketIntermediateDefinition&) =
      delete;
  BucketIntermediateDefinition(BucketIntermediateDefinition&&);

  static void RegisterJSONConverter(
      base::JSONValueConverter<BucketIntermediateDefinition>* converter);

  base::Value source;
  std::vector<std::unique_ptr<int>> buckets;
};

// Intermediate that converts numeric values to bucket indices using
// configurable thresholds.
class BucketIntermediate : public RemoteMetricIntermediate {
 public:
  BucketIntermediate(BucketIntermediateDefinition definition,
                     Delegate* delegate);
  ~BucketIntermediate() override;

  BucketIntermediate(const BucketIntermediate&) = delete;
  BucketIntermediate& operator=(const BucketIntermediate&) = delete;

  bool Init() override;
  base::Value Process() override;
  base::flat_set<std::string_view> GetStorageKeys() const override;
  void OnLastUsedProfilePrefsChanged(PrefService* profile_prefs) override;

 private:
  BucketIntermediateDefinition definition_;
  std::unique_ptr<RemoteMetricIntermediate> source_intermediate_;
  std::vector<int> bucket_values_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_MANAGED_BUCKET_INTERMEDIATE_H_
