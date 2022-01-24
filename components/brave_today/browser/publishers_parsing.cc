// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_today/browser/publishers_parsing.h"

#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"

namespace brave_news {

bool ParsePublisherList(const std::string& json, Publishers* publishers) {
  DCHECK(publishers);
  base::JSONReader::ValueWithError value_with_error =
      base::JSONReader::ReadAndReturnValueWithError(
          json, base::JSONParserOptions::JSON_PARSE_RFC);
  absl::optional<base::Value>& records_v = value_with_error.value;
  if (!records_v) {
    LOG(ERROR) << "Invalid response, could not parse JSON, JSON is: " << json;
    return false;
  }
  if (!records_v->is_list()) {
    return false;
  }
  for (const base::Value& publisher_raw : records_v->GetList()) {
    auto publisher = brave_news::mojom::Publisher::New();
    publisher->publisher_id = *publisher_raw.FindStringKey("publisher_id");
    publisher->publisher_name = *publisher_raw.FindStringKey("publisher_name");
    publisher->category_name = *publisher_raw.FindStringKey("category");
    publisher->is_enabled = publisher_raw.FindBoolKey("enabled").value_or(true);
    // TODO(petemill): Validate
    publishers->insert_or_assign(publisher->publisher_id, std::move(publisher));
  }
  return true;
}

}  // namespace brave_news
