// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_BRAVE_FARBLING_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_BRAVE_FARBLING_SERVICE_H_

#include <string>

#include "base/containers/flat_map.h"
#include "base/containers/span.h"
#include "base/memory/raw_ptr.h"
#include "base/token.h"
#include "components/content_settings/core/browser/host_content_settings_map.h"
#include "components/keyed_service/core/keyed_service.h"
#include "third_party/abseil-cpp/absl/random/random.h"

class GURL;

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace brave {

using FarblingPRNG = absl::random_internal::randen_engine<uint64_t>;

class BraveFarblingService : public KeyedService {
 public:
  explicit BraveFarblingService(
      HostContentSettingsMap* host_content_settings_map);
  ~BraveFarblingService() override;

  bool MakePseudoRandomGeneratorForURL(
      const GURL& url,
      base::span<const uint8_t> additional_entropy,
      FarblingPRNG* prng);

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Returns the farbling token for |url|, creating and storing one if absent.
  // |additional_entropy| is XOR-mixed into the returned token (but the stored
  // base token is unaffected). Equivalent to the free function
  // brave_shields::GetFarblingToken but operates on the in-memory per-profile
  // map rather than the HostContentSettingsMap-backed store.
  base::Token GetFarblingToken(const GURL& url,
                               base::span<const uint8_t> additional_entropy);

  // This is called when the clear browsing data is invoked on cookies and
  // history which aggressively resets all the farbling tokens keyed to
  // websites.
  void ResetFarblingTokensMap();

 private:
  const raw_ptr<HostContentSettingsMap> host_content_settings_map_;
  // In-memory eTLD+1-scoped farbling token store. Keyed by scheme + eTLD+1
  // pattern string. Cleared automatically when the
  // service is destroyed (i.e., on browser restart or profile destruction).
  base::flat_map<std::string, base::Token> farbling_tokens_map_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_BRAVE_FARBLING_SERVICE_H_
