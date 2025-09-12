/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "testing/gtest/include/gtest/gtest.h"

#define SetUp                                                \
  SetUpUnused() {}                                           \
  static constexpr char16_t kGeminiKeyword[] = u"@ask";      \
  const std::string kGeminiUrl = "https://search.brave.com"; \
  void SetUp

#include <components/omnibox/browser/featured_search_provider_unittest.cc>

#undef SetUp
