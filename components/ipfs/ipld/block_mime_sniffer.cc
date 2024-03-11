/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/ipld/block_mime_sniffer.h"
#include "absl/types/optional.h"
#include "brave/components/constants/network_constants.h"
#include "net/base/mime_sniffer.h"
#include "net/base/mime_util.h"
#include "url/gurl.h"

namespace ipfs::ipld {

BlockMimeSniffer::BlockMimeSniffer() = default;
BlockMimeSniffer::~BlockMimeSniffer() = default;

absl::optional<std::string> BlockMimeSniffer::GetMime(
    const std::string& file_extension,
    std::string_view content,
    const GURL& url) {
  std::string result;
  if (!file_extension.empty() &&
      net::GetWellKnownMimeTypeFromExtension(file_extension, &result)) {
    return result;
  }

  if (!content.empty() &&
      net::SniffMimeType(content, url, std::string(),
                         net::ForceSniffFileUrlsForHtml::kDisabled, &result)) {
    return result;
  }

  if ((result.empty() || result == kOctetStreamMimeType) &&
      net::SniffMimeTypeFromLocalData(content, &result)) {
    return result;
  }

  return absl::nullopt;
}

}  // namespace ipfs::ipld
