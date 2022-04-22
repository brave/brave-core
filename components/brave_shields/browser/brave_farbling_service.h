/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_FARBLING_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_FARBLING_SERVICE_H_

#include "third_party/abseil-cpp/absl/random/random.h"

class GURL;

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace brave {

typedef absl::randen_engine<uint64_t> FarblingPRNG;

class BraveFarblingService {
 public:
  BraveFarblingService();
  ~BraveFarblingService();

  uint64_t session_token(bool is_off_the_record);
  void set_session_tokens_for_testing(uint64_t session_token,
                                      uint64_t incognito_session_token);
  bool MakePseudoRandomGeneratorForURL(const GURL& url,
                                       bool is_off_the_record,
                                       FarblingPRNG* prng);

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

 private:
  uint64_t session_token_;
  uint64_t incognito_session_token_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_FARBLING_SERVICE_H_
