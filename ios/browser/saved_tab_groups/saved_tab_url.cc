// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/saved_tab_groups/saved_tab_url.h"

#include <string_view>

#include "base/strings/escape.h"
#include "components/saved_tab_groups/public/utils.h"
#include "url/gurl.h"

namespace brave {
namespace {

constexpr char kChromeNewTabURL[] = "chrome://newtab/";

GURL ChromeNewTabURL() {
  return GURL(kChromeNewTabURL);
}

bool IsBraveNewTab(const GURL& url) {
  if (!url.SchemeIs("about")) {
    return false;
  }
  const std::string_view spec = url.spec();
  return url.host() == "newtab" || spec == "about:newtab" ||
         spec == "about://newtab" || spec == "about://newtab/";
}

// `internal` is not a standard scheme, so the query is not always parsed.
std::string_view QueryText(const GURL& url) {
  if (url.has_query()) {
    return url.query();
  }
  const std::string_view spec = url.spec();
  const size_t question = spec.find('?');
  if (question == std::string_view::npos) {
    return std::string_view();
  }
  return spec.substr(question + 1);
}

// The page embedded in an `internal://` URL, from its `url` query param.
GURL EmbeddedInternalURL(const GURL& url) {
  const std::string_view query = QueryText(url);
  constexpr std::string_view kKey = "url=";
  size_t start = 0;
  while (start <= query.size()) {
    const size_t end = query.find('&', start);
    const std::string_view part = query.substr(
        start, end == std::string_view::npos ? std::string_view::npos
                                             : end - start);
    if (part.starts_with(kKey)) {
      // The embedded page is percent-encoded, including ':' and '/'.
      constexpr base::UnescapeRule::Type kRules =
          base::UnescapeRule::SPACES | base::UnescapeRule::PATH_SEPARATORS |
          base::UnescapeRule::URL_SPECIAL_CHARS_EXCEPT_PATH_SEPARATORS |
          base::UnescapeRule::REPLACE_PLUS_WITH_SPACE;
      return GURL(base::UnescapeURLComponent(part.substr(kKey.size()), kRules));
    }
    if (end == std::string_view::npos) {
      break;
    }
    start = end + 1;
  }
  return GURL();
}

GURL Unwrap(const GURL& input) {
  GURL url = input;
  // A wrapper can itself be wrapped. Bound the walk so a bad query cannot loop.
  for (int i = 0; i < 3 && url.SchemeIs("internal"); ++i) {
    if (url.spec().find("about/home") != std::string_view::npos) {
      return ChromeNewTabURL();
    }
    const GURL embedded = EmbeddedInternalURL(url);
    if (!embedded.is_valid()) {
      break;
    }
    url = embedded;
  }
  if (IsBraveNewTab(url)) {
    return ChromeNewTabURL();
  }
  return url;
}

}  // namespace

std::pair<GURL, std::u16string> NormalizeURLForSavedTabGroup(
    const GURL& url,
    const std::u16string& title) {
  const GURL normalized = Unwrap(url);
  if (!tab_groups::IsURLValidForSavedTabGroups(normalized)) {
    return tab_groups::GetDefaultUrlAndTitle();
  }
  return {normalized, title};
}

}  // namespace brave
