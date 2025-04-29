/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/media/media_web_contents_observer.h"
#include "media/mojo/mojom/media_player.mojom.h"

#define RequestPlay               \
  RequestFullscreen() override {} \
  void RequestPlay

#include "src/content/browser/media/session/media_session_controller_unittest.cc"
#undef RequestPlay
