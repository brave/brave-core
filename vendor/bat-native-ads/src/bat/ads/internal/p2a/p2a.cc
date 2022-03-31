/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/p2a/p2a.h"

#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {
namespace p2a {

void RecordEvent(const std::string& name,
                 const std::vector<std::string>& questions) {
  base::Value list(base::Value::Type::LIST);
  for (const auto& question : questions) {
    list.Append(question);
  }

  std::string json;
  base::JSONWriter::Write(list, &json);

  AdsClientHelper::Get()->RecordP2AEvent(name, mojom::P2AEventType::kListType,
                                         json);
}

}  // namespace p2a
}  // namespace ads
