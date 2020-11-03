/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/p2a/p2a.h"

#include <string>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ads/internal/ads_impl.h"

namespace ads {

P2A::P2A(
    AdsImpl* ads)
    : ads_(ads) {
  DCHECK(ads_);
}

P2A::~P2A() = default;

void P2A::RecordEvent(
    const std::string& name,
    const std::vector<std::string>& value) {
  base::Value list(base::Value::Type::LIST);
  for (const auto& item : value) {
    list.Append(item);
  }

  std::string json;
  base::JSONWriter::Write(list, &json);
  ads_->get_ads_client()->RecordP2AEvent(name, P2AEventType::kListType, json);
}

///////////////////////////////////////////////////////////////////////////////

}  // namespace ads
