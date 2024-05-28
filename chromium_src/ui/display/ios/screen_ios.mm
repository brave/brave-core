/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// Include this early so that the real include inside of screen_ios.mm is
// ignored due to header guard.
#include "base/apple/foundation_util.h"

#define ObjCCastStrict ObjCCast
#include "src/ui/display/ios/screen_ios.mm"
#undef ObjCCastStrict
