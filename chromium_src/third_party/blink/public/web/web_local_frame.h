// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_LOCAL_FRAME_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_LOCAL_FRAME_H_

#define SendAttributionSrc(...)        \
  SendAttributionSrc(__VA_ARGS__) = 0; \
  virtual void SetOriginForClearWindowNameCheck(const url::Origin&)

#include <third_party/blink/public/web/web_local_frame.h>  // IWYU pragma: export

#undef SendAttributionSrc

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_WEB_WEB_LOCAL_FRAME_H_
