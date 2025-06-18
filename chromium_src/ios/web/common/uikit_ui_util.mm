/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/apple/foundation_util.h"

// Replaces the strict cast to a standard cast since its possible for Brave to
// have a non-window scene thanks to the CarPlay support
#define ObjCCastStrict ObjCCast
#include "src/ios/web/common/uikit_ui_util.mm"
#undef ObjCCastStrict
