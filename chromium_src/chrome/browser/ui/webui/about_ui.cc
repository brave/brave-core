/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/re2/src/re2/re2.h"

#define CHROME_INTERNAL_URLS_TO_BRAVE                                 \
  const std::string chrome_header = "Chrome URLs";                    \
  const std::string brave_header = "Brave URLs";                      \
  const std::string chrome_pages_header = "List of Chrome URLs";      \
  const std::string brave_pages_header = "List of Brave URLs";        \
  const std::string chrome_internal_pages_header =                    \
      "List of chrome://internals pages";                             \
  const std::string brave_internal_pages_header =                     \
      "List of brave://internals pages";                              \
  const std::string chrome_url_list = ">chrome://";                   \
  const std::string brave_url_list = ">brave://";                     \
  RE2::GlobalReplace(&html, chrome_header, brave_header);             \
  RE2::GlobalReplace(&html, chrome_pages_header, brave_pages_header); \
  RE2::GlobalReplace(&html, chrome_internal_pages_header,             \
                     brave_internal_pages_header);                    \
  RE2::GlobalReplace(&html, chrome_url_list, brave_url_list);

#include "../../../../../../chrome/browser/ui/webui/about_ui.cc"
#undef CHROME_INTERNAL_URLS_TO_BRAVE
