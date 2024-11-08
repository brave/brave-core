// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/speedreader/common/url_readable_hints.h"

#include "base/no_destructor.h"
#include "base/strings/string_util.h"
#include "third_party/re2/src/re2/re2.h"
#include "url/gurl.h"

namespace {

// Regex pattern for paths like /blog/, /article/, /post/, hinting the page
// is a blog entry, magazine entry, or news article.
constexpr char kReadablePathSingleComponentHints[] =
    "(?i)/"
    "(blogs?|news|story|entry|articles?|posts?|amp|technology|politics|"
    "business)/";

// Regex pattern for matching URL paths of the form /YYYY/MM/DD/, which is
// extremely common for news websites.
constexpr char kReadablePathMultiComponentHints[] = "/\\d\\d\\d\\d/\\d\\d/";

constexpr char kReadableBlogSubdomain[] = "blog.";

}  // namespace

namespace speedreader {

bool IsURLLooksReadable(const GURL& url) {
  static base::NoDestructor<const re2::RE2> path_single_component_hints(
      kReadablePathSingleComponentHints);
  static base::NoDestructor<const re2::RE2> path_multi_component_hints(
      kReadablePathMultiComponentHints);
  DCHECK(path_single_component_hints->ok());
  DCHECK(path_multi_component_hints->ok());

  // Only HTTP is readable.
  if (!url.SchemeIsHTTPOrHTTPS())
    return false;

  // @pes research has shown basically no landing pages are readable.
  if (!url.has_path() || url.path() == "/")
    return false;

  if (url.host_piece().starts_with(kReadableBlogSubdomain)) {
    return true;
  }

  // Look for single components such as /blog/, /news/, /article/ and for
  // multi-path components like /YYYY/MM/DD
  if (re2::RE2::PartialMatch(url.path(), *path_single_component_hints) ||
      re2::RE2::PartialMatch(url.path(), *path_multi_component_hints)) {
    return true;
  }

  return false;
}

}  // namespace speedreader
