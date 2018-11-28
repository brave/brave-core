/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_IMPORTER_BRAVE_REFERRAL_H_
#define BRAVE_COMMON_IMPORTER_BRAVE_REFERRAL_H_

#include <string>

struct BraveReferral {
  BraveReferral();
  BraveReferral(const BraveReferral& other);
  ~BraveReferral();

  std::string promo_code;
  std::string download_id;
  uint64_t finalize_timestamp;
  std::string week_of_installation;
};

#endif  // BRAVE_COMMON_IMPORTER_BRAVE_REFERRAL_H_
