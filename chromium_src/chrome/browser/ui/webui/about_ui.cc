/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/re2/src/re2/re2.h"
#define CHROME_INTERNAL_URLS_TO_BRAVE                                 \
  const std::string CHROME = "Chrome";                                \
  const std::string BRAVE = "Brave";                                  \
  const std::string TAG_CHROME_PROTOCOL = ">chrome:";                 \
  const std::string TAG_BRAVE_PROTOCOL = ">brave:";                   \
  const std::string SPACE_CHROME_PROTOCOL = " chrome:";               \
  const std::string SPACE_BRAVE_PROTOCOL = " brave:";                 \
  RE2::GlobalReplace(&html, CHROME, BRAVE);                           \
  RE2::GlobalReplace(&html, TAG_CHROME_PROTOCOL, TAG_BRAVE_PROTOCOL); \
  RE2::GlobalReplace(&html, SPACE_CHROME_PROTOCOL, SPACE_BRAVE_PROTOCOL);

#include "../../../../../../chrome/browser/ui/webui/about_ui.cc"
#undef CHROME_INTERNAL_URLS_TO_BRAVE
