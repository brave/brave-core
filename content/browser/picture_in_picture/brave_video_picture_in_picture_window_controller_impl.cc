/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/content/browser/picture_in_picture/brave_video_picture_in_picture_window_controller_impl.h"

#include "content/public/browser/web_contents.h"

namespace content {

// static
BraveVideoPictureInPictureWindowControllerImpl*
BraveVideoPictureInPictureWindowControllerImpl::GetOrCreateForWebContents(
    WebContents* web_contents) {
  DCHECK(web_contents);

  // This is a no-op if the controller already exists.
  web_contents->SetUserData(
      UserDataKey(),
      base::WrapUnique(
          new BraveVideoPictureInPictureWindowControllerImpl(web_contents)));
  return static_cast<BraveVideoPictureInPictureWindowControllerImpl*>(
      FromWebContents(web_contents));
}

BraveVideoPictureInPictureWindowControllerImpl::
    ~BraveVideoPictureInPictureWindowControllerImpl() = default;

BraveVideoPictureInPictureWindowControllerImpl::
    BraveVideoPictureInPictureWindowControllerImpl(WebContents* web_contents)
    : VideoPictureInPictureWindowControllerImpl(web_contents) {}

}  // namespace content
