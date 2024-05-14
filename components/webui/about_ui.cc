// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/webui/about_ui.h"

#include <string_view>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "third_party/re2/src/re2/re2.h"

namespace brave {

std::string ReplaceAboutUIChromeURLs(std::string chrome_urls) {
  // Replace Chrome -> Brave.
  constexpr std::string_view kChromeHeader = "Chrome URLs";
  constexpr std::string_view kBraveHeader = "Brave URLs";
  constexpr std::string_view kChromePagesHeader = "List of Chrome URLs";
  constexpr std::string_view kBravePagesHeader = "List of Brave URLs";
  constexpr std::string_view kChromeInternalPagesHeader =
      "List of chrome://internals pages";
  constexpr std::string_view kBraveInternalPagesHeader =
      "List of brave://internals pages";
  constexpr std::string_view kChromeURLList = ">chrome://";
  constexpr std::string_view kBraveURLList = ">brave://";

  RE2::GlobalReplace(&chrome_urls, kChromeHeader, kBraveHeader);
  RE2::GlobalReplace(&chrome_urls, kChromePagesHeader, kBravePagesHeader);
  RE2::GlobalReplace(&chrome_urls, kChromeInternalPagesHeader,
                     kBraveInternalPagesHeader);
  RE2::GlobalReplace(&chrome_urls, kChromeURLList, kBraveURLList);

  // Remove some URLs.
  auto html_lines = base::SplitStringPiece(
      chrome_urls, "\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
  constexpr auto kURLsToRemove = base::MakeFixedFlatSet<std::string_view>({
      "brave://memories",
  });
  // URLs in html should be sorted so it's okay to iterate over sorted
  // kURLsToRemove.
  auto html_line_it = html_lines.begin();
  auto url_to_remove_it = kURLsToRemove.begin();
  while (html_line_it != html_lines.end() &&
         url_to_remove_it != kURLsToRemove.end()) {
    if (base::Contains(*html_line_it, *url_to_remove_it)) {
      html_line_it = html_lines.erase(html_line_it);
      ++url_to_remove_it;
    } else {
      ++html_line_it;
    }
  }

  return base::JoinString(html_lines, "\n");
}

}  // namespace brave
