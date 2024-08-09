/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_LANGUAGE_DETECTION_DETAILS_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_LANGUAGE_DETECTION_DETAILS_H_

#define RecordAcceptLanguageAndXmlHtmlLangMetric(...)    \
  RecordAcceptLanguageAndXmlHtmlLangMetric(__VA_ARGS__); \
  static void RecordAcceptLanguageAndXmlHtmlLangMetric_ChromiumImpl(__VA_ARGS__)

#include "src/third_party/blink/public/web/web_language_detection_details.h"  // IWYU pragma: export

#undef RecordAcceptLanguageAndXmlHtmlLangMetric

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_LANGUAGE_DETECTION_DETAILS_H_
