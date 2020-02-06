/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_CONFIRMATIONS_ISSUER_INFO_H_
#define BAT_CONFIRMATIONS_ISSUER_INFO_H_

#include <string>

#include "bat/confirmations/export.h"

namespace confirmations {

struct CONFIRMATIONS_EXPORT IssuerInfo {
  IssuerInfo();
  IssuerInfo(
      const IssuerInfo& info);
  ~IssuerInfo();

  std::string name;
  std::string public_key;
};

}  // namespace confirmations

#endif  // BAT_CONFIRMATIONS_ISSUER_INFO_H_
