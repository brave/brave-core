/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_FARBLING_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_FARBLING_SERVICE_H_

// https://github.com/brave/brave-browser/issues/21931
#include <random>

class GURL;

namespace brave {

class BraveFarblingService {
 public:
  BraveFarblingService();
  ~BraveFarblingService();

  uint64_t session_token(bool is_off_the_record);
  void set_session_tokens_for_testing();
  bool MakePseudoRandomGeneratorForURL(const GURL& url,
                                       bool is_off_the_record,
                                       std::mt19937_64* prng);

 private:
  uint64_t session_token_;
  uint64_t incognito_session_token_;
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_BRAVE_SHIELDS_BROWSER_BRAVE_FARBLING_SERVICE_H_
