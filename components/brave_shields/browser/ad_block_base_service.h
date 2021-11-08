/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_BASE_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_BASE_SERVICE_H_

#include <stdint.h>

#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "base/values.h"
#include "brave/components/brave_component_updater/browser/dat_file_util.h"
#include "brave/components/brave_shields/browser/base_brave_shields_service.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"

class AdBlockServiceTest;
class BraveAdBlockTPNetworkDelegateHelperTest;
class EphemeralStorage1pDomainBlockBrowserTest;
class PerfPredictorTabHelperTest;

using brave_component_updater::BraveComponent;
namespace adblock {
class Engine;
}

namespace brave_shields {

// The base class of the brave shields service in charge of ad-block
// checking and init.
class AdBlockBaseService : public BaseBraveShieldsService {
 public:
  using GetDATFileDataResult =
      brave_component_updater::LoadDATFileDataResult<adblock::Engine>;

  explicit AdBlockBaseService(BraveComponent::Delegate* delegate);
  AdBlockBaseService(const AdBlockBaseService&) = delete;
  AdBlockBaseService& operator=(const AdBlockBaseService&) = delete;
  ~AdBlockBaseService() override;

  void ShouldStartRequest(const GURL& url,
                          blink::mojom::ResourceType resource_type,
                          const std::string& tab_host,
                          bool aggressive_blocking,
                          bool* did_match_rule,
                          bool* did_match_exception,
                          bool* did_match_important,
                          std::string* replacement_url) override;
  absl::optional<std::string> GetCspDirectives(
      const GURL& url,
      blink::mojom::ResourceType resource_type,
      const std::string& tab_host);
  void AddResources(const std::string& resources);
  void EnableTag(const std::string& tag, bool enabled);
  bool TagExists(const std::string& tag);

  virtual absl::optional<base::Value> UrlCosmeticResources(
      const std::string& url);
  virtual absl::optional<base::Value> HiddenClassIdSelectors(
      const std::vector<std::string>& classes,
      const std::vector<std::string>& ids,
      const std::vector<std::string>& exceptions);

 protected:
  friend class ::AdBlockServiceTest;
  friend class ::BraveAdBlockTPNetworkDelegateHelperTest;
  friend class ::EphemeralStorage1pDomainBlockBrowserTest;
  friend class ::PerfPredictorTabHelperTest;

  bool Init() override;

  void GetDATFileData(const base::FilePath& dat_file_path,
                      bool deserialize = true,
                      base::OnceClosure callback = base::DoNothing());
  void AddKnownTagsToAdBlockInstance();
  void AddKnownResourcesToAdBlockInstance();
  void ResetForTest(const std::string& rules,
                    const std::string& resources = "",
                    bool include_redirect_urls = false);

  std::unique_ptr<adblock::Engine> ad_block_client_;

 private:
  void UpdateAdBlockClient(std::unique_ptr<adblock::Engine> ad_block_client);
  void OnGetDATFileData(base::OnceClosure callback,
                        GetDATFileDataResult result);
  void OnPreferenceChanges(const std::string& pref_name);

  std::set<std::string> tags_;
  std::string resources_;
  base::WeakPtrFactory<AdBlockBaseService> weak_factory_;
};

}  // namespace brave_shields

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_AD_BLOCK_BASE_SERVICE_H_
