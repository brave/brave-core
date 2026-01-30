/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_MANAGED_PROBE_INTERMEDIATE_H_
#define BRAVE_COMPONENTS_P3A_MANAGED_PROBE_INTERMEDIATE_H_

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_set.h"
#include "base/json/json_value_converter.h"
#include "base/metrics/histogram_base.h"
#include "base/metrics/statistics_recorder.h"
#include "base/values.h"
#include "brave/components/p3a/managed/remote_metric_intermediate.h"

namespace p3a {

class P3AProbeIntermediateTest;

class ProbeIntermediateDefinition {
 public:
  ProbeIntermediateDefinition();
  ~ProbeIntermediateDefinition();

  ProbeIntermediateDefinition(const ProbeIntermediateDefinition&) = delete;
  ProbeIntermediateDefinition& operator=(const ProbeIntermediateDefinition&) =
      delete;
  ProbeIntermediateDefinition(ProbeIntermediateDefinition&&);

  static void RegisterJSONConverter(
      base::JSONValueConverter<ProbeIntermediateDefinition>* converter);

  std::string histogram_name;
  std::vector<std::unique_ptr<int>> filter;
};

// Intermediate that observes histogram samples and reports the last captured
// value.
class ProbeIntermediate : public RemoteMetricIntermediate {
 public:
  ProbeIntermediate(ProbeIntermediateDefinition definition, Delegate* delegate);
  ~ProbeIntermediate() override;

  ProbeIntermediate(const ProbeIntermediate&) = delete;
  ProbeIntermediate& operator=(const ProbeIntermediate&) = delete;

  bool Init() override;
  base::Value Process() override;
  base::flat_set<std::string_view> GetStorageKeys() const override;
  void OnLastUsedProfilePrefsChanged(PrefService* profile_prefs) override;

 private:
  friend class P3AProbeIntermediateTest;

  FRIEND_TEST_ALL_PREFIXES(P3AProbeIntermediateTest,
                           InitFailsIfHistogramNameEmpty);
  FRIEND_TEST_ALL_PREFIXES(P3AProbeIntermediateTest, NoFilterCachesAnySample);
  FRIEND_TEST_ALL_PREFIXES(P3AProbeIntermediateTest,
                           FilterCachesOnlyMatchingSample);

  void OnHistogramSample(std::string_view histogram_name,
                         uint64_t name_hash,
                         base::HistogramBase::Sample32 sample);

  ProbeIntermediateDefinition definition_;
  std::unique_ptr<base::StatisticsRecorder::ScopedHistogramSampleObserver>
      scoped_observer_;
  std::optional<int> last_value_;
};

}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_MANAGED_PROBE_INTERMEDIATE_H_
