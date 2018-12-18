/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_CATALOG_PAYLOAD_INFO_H_
#define BAT_ADS_CATALOG_PAYLOAD_INFO_H_

#include <string>

namespace ads {

struct PayloadInfo {
  PayloadInfo() :
      body(""),
      title(""),
      target_url("") {}

  explicit PayloadInfo(const PayloadInfo& info) :
      body(info.body),
      title(info.title),
      target_url(info.target_url) {}

  ~PayloadInfo() {}

  std::string body;
  std::string title;
  std::string target_url;
};

}  // namespace ads

#endif  // BAT_ADS_CATALOG_PAYLOAD_INFO_H_
