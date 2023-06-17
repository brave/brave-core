/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_ENGINE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_ENGINE_H_

#include <stdint.h>

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/observer_list_types.h"
#include "base/sequence_checker.h"
#include "base/values.h"
#include "brave/components/adblock_rust_ffi/src/wrapper.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "url/gurl.h"

using brave_component_updater::DATFileDataBuffer;

namespace adblock {
class Engine;
}  // namespace adblock

class AdBlockServiceTest;
class BraveAdBlockTPNetworkDelegateHelperTest;
class EphemeralStorage1pDomainBlockBrowserTest;
class PerfPredictorTabHelperTest;

namespace brave_shields {

// Service managing an adblock engine.
class AdBlockEngine : public base::SupportsWeakPtr<AdBlockEngine> {
 public:
  using GetDATFileDataResult =
      brave_component_updater::LoadDATFileDataResult<adblock::Engine>;

  explicit AdBlockEngine(bool is_default_engine);
  AdBlockEngine(const AdBlockEngine&) = delete;
  AdBlockEngine& operator=(const AdBlockEngine&) = delete;
  ~AdBlockEngine();

  bool IsDefaultEngine() { return is_default_engine_; }

  void ShouldStartRequest(const GURL& url,
                          blink::mojom::ResourceType resource_type,
                          const std::string& tab_host,
                          bool aggressive_blocking,
                          bool* did_match_rule,
                          bool* did_match_exception,
                          bool* did_match_important,
                          std::string* mock_data_url,
                          std::string* rewritten_url);
  absl::optional<std::string> GetCspDirectives(
      const GURL& url,
      blink::mojom::ResourceType resource_type,
      const std::string& tab_host);
  void UseResources(const std::string& resources);
  void EnableTag(const std::string& tag, bool enabled);
  bool TagExists(const std::string& tag);

  base::Value::Dict GetDebugInfo();
  void DiscardRegex(uint64_t regex_id);
  void SetupDiscardPolicy(const adblock::RegexManagerDiscardPolicy& policy);

  base::Value::Dict UrlCosmeticResources(const std::string& url);
  base::Value::List HiddenClassIdSelectors(
      const std::vector<std::string>& classes,
      const std::vector<std::string>& ids,
      const std::vector<std::string>& exceptions);

  void Load(bool deserialize,
            const DATFileDataBuffer& dat_buf,
            const std::string& resources_json);

  class TestObserver : public base::CheckedObserver {
   public:
    virtual void OnEngineUpdated() = 0;
  };

  void AddObserverForTest(TestObserver* observer);
  void RemoveObserverForTest();

 protected:
  void AddKnownTagsToAdBlockInstance();
  void UpdateAdBlockClient(std::unique_ptr<adblock::Engine> ad_block_client,
                           const std::string& resources_json);
  void OnListSourceLoaded(const DATFileDataBuffer& filters,
                          const std::string& resources_json);

  void OnDATLoaded(const DATFileDataBuffer& dat_buf,
                   const std::string& resources_json);

  std::unique_ptr<adblock::Engine> ad_block_client_
      GUARDED_BY_CONTEXT(sequence_checker_);

 private:
  friend class ::AdBlockServiceTest;
  friend class ::BraveAdBlockTPNetworkDelegateHelperTest;
  friend class ::EphemeralStorage1pDomainBlockBrowserTest;
  friend class ::PerfPredictorTabHelperTest;

  std::set<std::string> tags_ GUARDED_BY_CONTEXT(sequence_checker_);
  absl::optional<adblock::RegexManagerDiscardPolicy> regex_discard_policy_
      GUARDED_BY_CONTEXT(sequence_checker_);

  raw_ptr<TestObserver> test_observer_ = nullptr;

  bool is_default_engine_;

  SEQUENCE_CHECKER(sequence_checker_);
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_ENGINE_H_
