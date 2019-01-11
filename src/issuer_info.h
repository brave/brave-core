/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_ISSUER_INFO_H_
#define BAT_ADS_ISSUER_INFO_H_

#include <string>

namespace ads {

struct IssuerInfo {
  IssuerInfo() :
      name(""),
      public_key("") {}

  explicit IssuerInfo(const std::string& public_key) :
      name(""),
      public_key(public_key) {}

  IssuerInfo(const IssuerInfo& info) :
      name(info.name),
      public_key(info.public_key) {}

  ~IssuerInfo() {}

  std::string name;
  std::string public_key;
};

}  // namespace ads

#endif  // BAT_ADS_ISSUER_INFO_H_
