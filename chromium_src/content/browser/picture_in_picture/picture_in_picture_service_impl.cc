// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/content/browser/picture_in_picture/brave_video_picture_in_picture_window_controller_impl.h"
#include "content/public/browser/web_contents.h"

#define VideoPictureInPictureWindowControllerImpl \
  BraveVideoPictureInPictureWindowControllerImpl
#include "src/content/browser/picture_in_picture/picture_in_picture_service_impl.cc"
#undef VideoPictureInPictureWindowControllerImpl
