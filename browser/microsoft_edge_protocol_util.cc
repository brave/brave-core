/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/microsoft_edge_protocol_util.h"

#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/sys_string_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "url/url_util.h"

namespace {

std::string DecodeURL(const std::string& url) {
  url::RawCanonOutputT<char16_t> unescaped;
  url::DecodeURLEscapeSequences(url.data(), url.size(),
                                url::DecodeURLMode::kUTF8OrIsomorphic,
                                &unescaped);

  std::string output;
  base::UTF16ToUTF8(unescaped.data(), unescaped.length(), &output);
  return output;
}

}  // namespace

absl::optional<std::string> GetURLFromMSEdgeProtocol(
    const std::wstring& command_line_arg) {
  constexpr wchar_t kMSEdgeProtocol[] = L"microsoft-edge:";
  if (base::StartsWith(command_line_arg, kMSEdgeProtocol)) {
    const std::wstring protocol_arg =
        command_line_arg.substr(wcslen(kMSEdgeProtocol));
    // Cortana(window search) passes link url in the query.
    const bool has_query = protocol_arg[0] == '?';
    if (has_query) {
      // query stores string after '?'.
      const std::wstring query = protocol_arg.substr(1);

      // Find URL key from cortana query.
      for (const auto& cur :
           base::SplitString(base::SysWideToUTF8(query), "&",
                             base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL)) {
        constexpr char kCortanaURLKey[] = "url=";
        if (base::StartsWith(cur, kCortanaURLKey)) {
          return DecodeURL(cur.substr(strlen(kCortanaURLKey)));
        }
      }
    } else {
      // If it's not a query string, we assume |protocol_arg| is url.
      // If it's not a valid url, this url will be ignored at
      // GetURLsFromCommandLine().
      return DecodeURL(base::SysWideToUTF8(protocol_arg));
    }
  }

  return absl::nullopt;
}
