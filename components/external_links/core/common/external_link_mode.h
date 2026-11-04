/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_EXTERNAL_LINKS_CORE_COMMON_EXTERNAL_LINK_MODE_H_
#define BRAVE_COMPONENTS_EXTERNAL_LINKS_CORE_COMMON_EXTERNAL_LINK_MODE_H_

namespace external_links {

// How a link opened from another app is loaded. Persisted as the integer value
// of BRAVE_EXTERNAL_LINK_MODE, so values must not be reordered or reused.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.external_links
enum class ExternalLinkMode {
  kCurrent = 0,
  kAsk = 1,
  kRegular = 2,
  kPrivateBrowsing = 3,
  kModal = 4,
  kMaxValue = kModal,
};

// What the embedder should do with a link opened from another app.
// GENERATED_JAVA_ENUM_PACKAGE: org.chromium.chrome.browser.external_links
enum class ExternalLinkAction {
  kOpenCurrent,
  kShowOnboarding,
  kShowPrompt,
  kOpenRegular,
  kOpenPrivate,
  kOpenModal,
};

}  // namespace external_links

#endif  // BRAVE_COMPONENTS_EXTERNAL_LINKS_CORE_COMMON_EXTERNAL_LINK_MODE_H_
