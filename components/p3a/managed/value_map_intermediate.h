/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_MANAGED_VALUE_MAP_INTERMEDIATE_H_
#define BRAVE_COMPONENTS_P3A_MANAGED_VALUE_MAP_INTERMEDIATE_H_

#include <memory>

#include "base/containers/flat_set.h"
#include "base/json/json_value_converter.h"
#include "base/values.h"
#include "brave/components/p3a/managed/remote_metric_intermediate.h"

namespace p3a {

class ValueMapIntermediateDefinition {
 public:
  ValueMapIntermediateDefinition();
  ~ValueMapIntermediateDefinition();

  ValueMapIntermediateDefinition(const ValueMapIntermediateDefinition&) =
      delete;
  ValueMapIntermediateDefinition& operator=(
      const ValueMapIntermediateDefinition&) = delete;
  ValueMapIntermediateDefinition(ValueMapIntermediateDefinition&&);

  static void RegisterJSONConverter(
      base::JSONValueConverter<ValueMapIntermediateDefinition>* converter);

  base::Value source;
  base::Value::Dict map;
};

// Intermediate that transforms values from a source using a predefined mapping
// table.
class ValueMapIntermediate : public RemoteMetricIntermediate {
 public:
  ValueMapIntermediate(ValueMapIntermediateDefinition definition,
                       Delegate* delegate);
  ~ValueMapIntermediate() override;

  ValueMapIntermediate(const ValueMapIntermediate&) = delete;
  ValueMapIntermediate& operator=(const ValueMapIntermediate&) = delete;

  bool Init() override;
  base::Value Process() override;
  base::flat_set<std::string_view> GetStorageKeys() const override;
  void OnLastUsedProfilePrefsChanged(PrefService* profile_prefs) override;

 private:
  ValueMapIntermediateDefinition definition_;
  std::unique_ptr<RemoteMetricIntermediate> source_intermediate_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_MANAGED_VALUE_MAP_INTERMEDIATE_H_
