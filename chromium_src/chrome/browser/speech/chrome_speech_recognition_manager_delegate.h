/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SPEECH_CHROME_SPEECH_RECOGNITION_MANAGER_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SPEECH_CHROME_SPEECH_RECOGNITION_MANAGER_DELEGATE_H_

#define CheckRenderFrameType                                             \
  CheckRenderFrameType_ChromiumImpl(                                     \
      base::OnceCallback<void(bool ask_user, bool is_allowed)> callback, \
      int render_process_id, int render_frame_id);                       \
  static void CheckRenderFrameType

#include "src/chrome/browser/speech/chrome_speech_recognition_manager_delegate.h"  // IWYU pragma: export

#undef CheckRenderFrameType

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_SPEECH_CHROME_SPEECH_RECOGNITION_MANAGER_DELEGATE_H_
