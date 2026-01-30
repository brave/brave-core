/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_MANAGED_PREF_INTERMEDIATE_H_
#define BRAVE_COMPONENTS_P3A_MANAGED_PREF_INTERMEDIATE_H_

#include <string>

#include "base/containers/flat_set.h"
#include "base/json/json_value_converter.h"
#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/p3a/managed/remote_metric_intermediate.h"
#include "components/prefs/pref_change_registrar.h"

class PrefService;

namespace p3a {

class PrefIntermediateDefinition {
 public:
  PrefIntermediateDefinition();
  ~PrefIntermediateDefinition();

  PrefIntermediateDefinition(const PrefIntermediateDefinition&) = delete;
  PrefIntermediateDefinition& operator=(const PrefIntermediateDefinition&) =
      delete;
  PrefIntermediateDefinition(PrefIntermediateDefinition&&);

  static void RegisterJSONConverter(
      base::JSONValueConverter<PrefIntermediateDefinition>* converter);

  std::string pref_name;
  bool use_profile_prefs = false;
};

// Intermediate that monitors and reports preference values, with automatic
// updates on changes.
class PrefIntermediate : public RemoteMetricIntermediate {
 public:
  PrefIntermediate(PrefIntermediateDefinition definition,
                   PrefService* local_state,
                   PrefService* profile_prefs,
                   Delegate* delegate);
  ~PrefIntermediate() override;

  PrefIntermediate(const PrefIntermediate&) = delete;
  PrefIntermediate& operator=(const PrefIntermediate&) = delete;

  bool Init() override;
  base::Value Process() override;
  base::flat_set<std::string_view> GetStorageKeys() const override;

  void OnLastUsedProfilePrefsChanged(PrefService* profile_prefs) override;

 private:
  void OnPrefChanged();
  PrefService* GetPrefService() const;

  PrefIntermediateDefinition definition_;
  raw_ptr<PrefService> local_state_;
  raw_ptr<PrefService> profile_prefs_;
  PrefChangeRegistrar pref_change_registrar_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_MANAGED_PREF_INTERMEDIATE_H_
