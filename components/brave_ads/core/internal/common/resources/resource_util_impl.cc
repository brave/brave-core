/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/resources/resource_util_impl.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/json/json_reader.h"

namespace brave_ads {

std::optional<base::DictValue> ReadResourceComponentFileToDict(
    base::File file) {
  if (!file.IsValid()) {
    return std::nullopt;
  }

  // `content` can be up to 10 MB, so we keep the scope of this object to this
  // block to release its memory as soon as possible.

  std::string content;
  const base::ScopedFILE scoped_file(base::FileToFILE(std::move(file), "rb"));
  if (!base::ReadStreamToString(scoped_file.get(), &content)) {
    return std::nullopt;
  }

  return base::JSONReader::ReadDict(content, base::JSON_PARSE_RFC);
}

}  // namespace brave_ads
