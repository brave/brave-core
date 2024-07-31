/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "third_party/blink/public/web/web_language_detection_details.h"

// Don't run this upstream telemetry function, because it unnecessarily
// triggers the fingerprinting protection UI.
#define RecordAcceptLanguageAndXmlHtmlLangMetric(...)      \
  RecordAcceptLanguageAndXmlHtmlLangMetric(__VA_ARGS__) {} \
  void WebLanguageDetectionDetails::                       \
      RecordAcceptLanguageAndXmlHtmlLangMetric_ChromiumImpl(__VA_ARGS__)

#include "src/third_party/blink/renderer/core/exported/web_language_detection_details.cc"

#undef RecordAcceptLanguageAndXmlHtmlLangMetric
