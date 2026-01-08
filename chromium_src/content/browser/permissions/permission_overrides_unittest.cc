/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

// Upstream hardcoded kPermissionsCount to be 40, which doesn't take into
// account all Brave's permissions. This define is not great, but perhaps will
// hold. See
// chromium_src/third_party/blink/public/common/permissions/permission_utils.h
constexpr size_t kBraveExtraPermissionsCount = 13;
#define SizeIs(VALUE)                                          \
  SizeIs(VALUE == kPermissionsCount                            \
             ? kPermissionsCount + kBraveExtraPermissionsCount \
             : VALUE)

#include <content/browser/permissions/permission_overrides_unittest.cc>
#undef SizeIs
