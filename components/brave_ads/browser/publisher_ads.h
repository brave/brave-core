/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_PUBLISHER_ADS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_PUBLISHER_ADS_H_

#include <string>

namespace brave_ads {

enum PublisherAdEventType {
  kViewed = 0,
  kClicked
};

struct PublisherAdInfo {
  PublisherAdInfo();
  PublisherAdInfo(
      const PublisherAdInfo& info);
  ~PublisherAdInfo();

  std::string creative_instance_id;
  std::string creative_set_id;
  std::string category;
  std::string size;
  std::string creative_url;
  std::string target_url;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_PUBLISHER_ADS_H_
