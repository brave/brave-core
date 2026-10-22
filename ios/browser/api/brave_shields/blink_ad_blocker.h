// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_BLINK_AD_BLOCKER_H_
#define BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_BLINK_AD_BLOCKER_H_

#include <optional>

#include "base/functional/callback.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "brave/components/brave_shields/core/common/adblock/rs/src/lib.rs.h"
#include "third_party/rust/cxx/v1/cxx.h"
#include "url/gurl.h"

namespace network {
struct ResourceRequest;
}  // namespace network

namespace brave_shields {

class AdBlockFiltersProviderManager;

// Blocks Blink subresource loads using the default filter lists. WebKit builds
// block through content rule lists instead.
class BlinkAdBlocker : public AdBlockFiltersProvider::Observer {
 public:
  explicit BlinkAdBlocker(AdBlockFiltersProviderManager* manager);
  BlinkAdBlocker(const BlinkAdBlocker&) = delete;
  BlinkAdBlocker& operator=(const BlinkAdBlocker&) = delete;
  ~BlinkAdBlocker() override;

  void SetOnBlocked(base::RepeatingCallback<void(const GURL&)> on_blocked);

  // AdBlockFiltersProvider::Observer:
  void OnChanged(bool is_default_engine) override;

 private:
  void OnFilterSetLoaded(
      base::OnceCallback<void(rust::Box<adblock::FilterSet>*)> load);
  void OnEngineBuilt(std::optional<rust::Box<adblock::Engine>> engine);
  bool ShouldBlock(const network::ResourceRequest& request);

  raw_ptr<AdBlockFiltersProviderManager> manager_;
  std::optional<rust::Box<adblock::Engine>> engine_;
  base::RepeatingCallback<void(const GURL&)> on_blocked_;
  base::WeakPtrFactory<BlinkAdBlocker> weak_factory_{this};
};

}  // namespace brave_shields

#endif  // BRAVE_IOS_BROWSER_API_BRAVE_SHIELDS_BLINK_AD_BLOCKER_H_
