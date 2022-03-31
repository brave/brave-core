/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_INLINE_CONTENT_AD_INFO_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_INLINE_CONTENT_AD_INFO_H_

#include <string>

#include "bat/ads/ad_info.h"
#include "bat/ads/export.h"

namespace base {
class DictionaryValue;
class Value;
}  // namespace base

namespace ads {

struct ADS_EXPORT InlineContentAdInfo final : AdInfo {
  InlineContentAdInfo();
  InlineContentAdInfo(const InlineContentAdInfo& info);
  ~InlineContentAdInfo();

  bool operator==(const InlineContentAdInfo& rhs) const;
  bool operator!=(const InlineContentAdInfo& rhs) const;

  bool IsValid() const;

  base::DictionaryValue ToValue() const;
  bool FromValue(const base::Value& value);

  std::string ToJson() const;
  bool FromJson(const std::string& json);

  std::string title;
  std::string description;
  std::string image_url;
  std::string dimensions;
  std::string cta_text;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_INCLUDE_BAT_ADS_INLINE_CONTENT_AD_INFO_H_
