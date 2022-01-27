/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/microsoft_edge_protocol_util.h"

#include <string>

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "url/url_util.h"

namespace {

std::string DecodeURL(base::StringPiece url) {
  url::RawCanonOutputT<char16_t> unescaped;
  url::DecodeURLEscapeSequences(url.data(), url.size(),
                                url::DecodeURLMode::kUTF8OrIsomorphic,
                                &unescaped);

  std::string output;
  base::UTF16ToUTF8(unescaped.data(), unescaped.length(), &output);
  return output;
}

}  // namespace

absl::optional<GURL> GetURLFromMSEdgeProtocol(
    base::WStringPiece command_line_arg) {
  constexpr base::WStringPiece kMSEdgeProtocol = L"microsoft-edge:";
  if (!base::StartsWith(command_line_arg, kMSEdgeProtocol))
    return absl::nullopt;

  // From now on, it's "microsoft-edge:" protocol args.
  base::WStringPiece protocol_arg = command_line_arg;
  protocol_arg.remove_prefix(kMSEdgeProtocol.length());
  // Handle protocol's arg is empty.
  if (protocol_arg.empty())
    return absl::nullopt;

  // query stores string after '?'.
  const bool has_query = protocol_arg[0] == '?';
  if (!has_query) {
    // If it's not a query string, we assume |protocol_arg| is url.
    GURL url(DecodeURL(base::WideToUTF8(protocol_arg)));
    if (url.is_valid())
      return url;
    return absl::nullopt;
  }

  // Remove first character '?'.
  protocol_arg.remove_prefix(1);

  // Windows Search passes link url in the query.
  // Find URL key from cortana query.
  for (const auto& cur :
       base::SplitString(base::WideToUTF8(protocol_arg), "&",
                         base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL)) {
    constexpr base::StringPiece kCortanaURLKey = "url=";
    if (!base::StartsWith(cur, kCortanaURLKey))
      continue;

    // We assume query includes only one url key.
    GURL url(DecodeURL(cur.substr(kCortanaURLKey.length())));
    if (url.is_valid())
      return url;
    break;
  }

  return absl::nullopt;
}
