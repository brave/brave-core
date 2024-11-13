/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/browser/api/history/importer/history_json_reader.h"

#include <optional>
#include <utility>

#include "base/files/file_util.h"
#include "base/functional/callback.h"
#include "base/i18n/icu_string_conversions.h"
#include "base/json/json_reader.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "net/base/data_url.h"
#include "url/gurl.h"

namespace history_json_reader {

void ImportHistoryFile(
    base::RepeatingCallback<bool(void)> cancellation_callback,
    base::RepeatingCallback<bool(const GURL&)> valid_url_callback,
    const base::FilePath& file_path,
    std::vector<history::URLRow>* history_items) {
  CHECK(history_items);

  std::string file_data;
  CHECK(base::ReadFileToString(file_path, &file_data));

  if (internal::ParseHistoryItems(file_data, history_items,
                                  std::move(cancellation_callback),
                                  std::move(valid_url_callback))) {
    // Handle additional processing here if necessary.
  }
}

namespace internal {

bool ParseHistoryItems(
    const std::string& json_data,
    std::vector<history::URLRow>* history_items,
    base::RepeatingCallback<bool(void)> cancellation_callback,
    base::RepeatingCallback<bool(const GURL&)> valid_url_callback) {
  CHECK(!json_data.empty());
  CHECK(history_items);

  std::optional<base::Value> parsed_json = base::JSONReader::Read(json_data);
  if (!parsed_json || !parsed_json->is_dict()) {
    return false;  // History file format is incorrect. Expected
                   // Structure/Dictionary (meta-data).
  }

  const base::Value::Dict& meta_data = parsed_json->GetDict();
  auto* items = meta_data.FindList("history");
  if (!items) {
    return false;  // History file format is incorrect. Expected Array/List
                   // (history).
  }

  for (const base::Value& item : *items) {
    // Handle Import cancelled
    if (!cancellation_callback.is_null() && cancellation_callback.Run()) {
      return false;
    }

    // History file format is incorrect. Expected Dictionary for each item.
    if (!item.is_dict()) {
      continue;
    }

    history::URLRow url_row;
    const base::Value::Dict& dict = item.GetDict();

    // URL is non-optional
    auto* url_string = dict.FindString("url");
    if (!url_string || url_string->empty()) {
      return false;
    }

    GURL url = GURL(*url_string);
    if (valid_url_callback.is_null() || valid_url_callback.Run(url)) {
      url_row.set_url(url);
    } else {
      continue;  // Ignore this item
    }

    // Title is optional
    auto* title = dict.FindString("title");
    if (title && !title->empty()) {
      url_row.set_title(base::UTF8ToUTF16(*title));
    }

    // time_usec_value is non-optional
    auto time_usec = dict.FindDouble("time_usec");
    if (!time_usec.has_value()) {
      continue;
    }

    url_row.set_last_visit(base::Time::UnixEpoch() +
                           base::Microseconds(*time_usec));

    // visit_count is non-optional
    auto visit_count = dict.FindInt("visit_count");
    if (!visit_count.has_value()) {
      continue;
    }

    url_row.set_visit_count(*visit_count);
    history_items->push_back(url_row);
  }

  return true;
}

}  // namespace internal

}  // namespace history_json_reader
