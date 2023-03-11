/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_RENDER_FRAME_HOST_IMPL_H_
#define BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_RENDER_FRAME_HOST_IMPL_H_

#include "content/public/browser/render_frame_host.h"
#include "third_party/blink/public/mojom/frame/frame.mojom.h"

#define CopyImageAt                                                            \
  GetImageAt(int x, int y, base::OnceCallback<void(const SkBitmap&)> callback) \
      override;                                                                \
  void CopyImageAt

#include "src/content/browser/renderer_host/render_frame_host_impl.h"  // IWYU pragma: export

#undef CopyImageAt

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_RENDER_FRAME_HOST_IMPL_H_
