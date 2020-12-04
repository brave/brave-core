/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_TARGETING_RESOURCES_RESOURCE_H_
#define BAT_ADS_INTERNAL_AD_TARGETING_RESOURCES_RESOURCE_H_

namespace ads {
namespace ad_targeting {
namespace resource {

template <class T>
class Resource {
 public:
  virtual ~Resource() = default;

  virtual bool IsInitialized() const = 0;

  virtual T get() const = 0;
};

}  // namespace resource
}  // namespace ad_targeting
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_TARGETING_RESOURCES_RESOURCE_H_
