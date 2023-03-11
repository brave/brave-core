/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCAL_FRAME_MOJO_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCAL_FRAME_MOJO_HANDLER_H_

#include "third_party/blink/public/mojom/frame/frame.mojom-blink.h"

#define CopyImageAt                                                       \
  GetImageAt(const gfx::Point& window_point, GetImageAtCallback callback) \
      final;                                                              \
  void CopyImageAt

#include "src/third_party/blink/renderer/core/frame/local_frame_mojo_handler.h"  // IWYU pragma: export

#undef CopyImageAt

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_FRAME_LOCAL_FRAME_MOJO_HANDLER_H_
