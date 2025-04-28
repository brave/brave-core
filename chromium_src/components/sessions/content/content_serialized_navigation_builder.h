/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_SESSIONS_CONTENT_CONTENT_SERIALIZED_NAVIGATION_BUILDER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_SESSIONS_CONTENT_CONTENT_SERIALIZED_NAVIGATION_BUILDER_H_

#define FromNavigationEntry(...)                 \
  FromNavigationEntry_ChromiumImpl(__VA_ARGS__); \
  static sessions::SerializedNavigationEntry FromNavigationEntry(__VA_ARGS__)

#include <components/sessions/content/content_serialized_navigation_builder.h>  // IWYU pragma: export

#undef FromNavigationEntry

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_SESSIONS_CONTENT_CONTENT_SERIALIZED_NAVIGATION_BUILDER_H_
