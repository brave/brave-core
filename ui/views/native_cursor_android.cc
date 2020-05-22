// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/views/native_cursor.h"

#include "ui/base/cursor/cursor.h"

namespace views {

gfx::NativeCursor GetNativeIBeamCursor() {
  return gfx::kNullCursor;
}

gfx::NativeCursor GetNativeHandCursor() {
  return gfx::kNullCursor;
}

gfx::NativeCursor GetNativeColumnResizeCursor() {
  return gfx::kNullCursor;
}

gfx::NativeCursor GetNativeEastWestResizeCursor() {
  return gfx::kNullCursor;
}

gfx::NativeCursor GetNativeNorthSouthResizeCursor() {
  return gfx::kNullCursor;
}

}  // namespace views
