// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_ENGINE_WRAPPER_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_ENGINE_WRAPPER_H_

#include <stdint.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/sequence_checker.h"
#include "base/thread_annotations.h"
#include "base/values.h"
#include "brave/components/brave_shields/core/browser/ad_block_resource_provider.h"
#include "brave/components/brave_shields/core/common/adblock/rs/src/lib.rs.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "url/gurl.h"

namespace brave_shields {

class AdBlockEngine;

// Wrapper combining the default and additional adblock engines.
// This class should only be accessed from the adblock task runner.
class AdBlockEngineWrapper {
 public:
  AdBlockEngineWrapper(std::unique_ptr<AdBlockEngine> default_engine,
                       std::unique_ptr<AdBlockEngine> additional_engine);
  AdBlockEngineWrapper(const AdBlockEngineWrapper&) = delete;
  AdBlockEngineWrapper& operator=(const AdBlockEngineWrapper&) = delete;
  ~AdBlockEngineWrapper();

  adblock::BlockerResult ShouldStartRequest(
      const GURL& url,
      blink::mojom::ResourceType resource_type,
      const std::string& tab_host,
      bool aggressive_blocking,
      bool previously_matched_rule,
      bool previously_matched_exception,
      bool previously_matched_important);

  void OnResourcesLoaded(
      bool is_default_engine,
      std::unique_ptr<rust::Box<adblock::FilterSet>> filter_set,
      AdblockResourceStorageBox storage);

  std::optional<std::string> GetCspDirectives(
      const GURL& url,
      blink::mojom::ResourceType resource_type,
      const std::string& tab_host);

  void UseResources(const adblock::BraveCoreResourceStorage& storage);
  void EnableTag(const std::string& tag, bool enabled);
  bool TagExists(const std::string& tag);

  std::pair<base::DictValue, base::DictValue> GetDebugInfo();
  void DiscardRegex(uint64_t regex_id);
  void SetupDiscardPolicy(const adblock::RegexManagerDiscardPolicy& policy);

  base::DictValue UrlCosmeticResources(const std::string& url,
                                       bool aggressive_blocking);
  base::DictValue HiddenClassIdSelectors(
      const std::vector<std::string>& classes,
      const std::vector<std::string>& ids,
      const std::vector<std::string>& exceptions);

  AdBlockEngine& default_engine() {
    return *TS_UNCHECKED_READ(default_engine_);
  }

  AdBlockEngine& additional_filters_engine() {
    return *TS_UNCHECKED_READ(additional_filters_engine_);
  }

  static void StripProceduralFilters(base::DictValue& resources);
  static void MergeResourcesInto(base::DictValue from,
                                 base::DictValue& into,
                                 bool force_hide);

 private:
  const std::unique_ptr<AdBlockEngine> default_engine_
      GUARDED_BY_CONTEXT(sequence_checker_);
  const std::unique_ptr<AdBlockEngine> additional_filters_engine_
      GUARDED_BY_CONTEXT(sequence_checker_);

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_AD_BLOCK_ENGINE_WRAPPER_H_
