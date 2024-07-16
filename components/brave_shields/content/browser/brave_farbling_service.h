// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_BRAVE_FARBLING_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_BRAVE_FARBLING_SERVICE_H_

#include "base/memory/raw_ptr.h"
#include "components/keyed_service/core/keyed_service.h"
#include "third_party/abseil-cpp/absl/random/random.h"

class GURL;

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace brave {

typedef absl::randen_engine<uint64_t> FarblingPRNG;

class BraveFarblingService : public KeyedService {
 public:
  BraveFarblingService();
  ~BraveFarblingService() override;

  uint64_t session_token();
  void set_session_tokens_for_testing(uint64_t session_token);
  bool MakePseudoRandomGeneratorForURL(const GURL& url, FarblingPRNG* prng);

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

 private:
  uint64_t session_token_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_CONTENT_BROWSER_BRAVE_FARBLING_SERVICE_H_
