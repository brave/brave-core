// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_IOS_WEB_WEBUI_MOJO_FACADE_H_
#define BRAVE_CHROMIUM_SRC_IOS_WEB_WEBUI_MOJO_FACADE_H_

#include <WebKit/WebKit.h>

#include "url/gurl.h"

#define HandleMojoMessage                                                   \
  Dummy();                                                                  \
  bool IsWebUIMessageAllowedForFrame(const GURL& origin, NSString* prompt); \
  std::string HandleMojoMessage

// Override OnWatcherCallback to have a frame_id parameter
#define OnWatcherCallback                                               \
  OnWatcherCallback_BraveImpl(int callback_id, int watch_id,            \
                              std::string frame_id, MojoResult result); \
  void OnWatcherCallback

#include <ios/web/webui/mojo_facade.h>  // IWYU pragma: export

#undef OnWatcherCallback

#undef HandleMojoMessage

#endif  // BRAVE_CHROMIUM_SRC_IOS_WEB_WEBUI_MOJO_FACADE_H_
