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

#define embedding_token_                                              \
  embedding_token_;                                                   \
                                                                      \
  std::optional<base::UnguessableToken> ephemeral_storage_token_;     \
  bool ephemeral_storage_token_set_ = false;                          \
  void SetEphemeralStorageToken(const url::Origin& top_frame_origin); \
  std::optional<base::UnguessableToken> GetEphemeralStorageToken() const

#include "src/content/browser/renderer_host/render_frame_host_impl.h"  // IWYU pragma: export

#undef embedding_token_
#undef CopyImageAt

#endif  // BRAVE_CHROMIUM_SRC_CONTENT_BROWSER_RENDERER_HOST_RENDER_FRAME_HOST_IMPL_H_
