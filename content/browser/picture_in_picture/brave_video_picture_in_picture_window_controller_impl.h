/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CONTENT_BROWSER_PICTURE_IN_PICTURE_BRAVE_VIDEO_PICTURE_IN_PICTURE_WINDOW_CONTROLLER_IMPL_H_
#define BRAVE_CONTENT_BROWSER_PICTURE_IN_PICTURE_BRAVE_VIDEO_PICTURE_IN_PICTURE_WINDOW_CONTROLLER_IMPL_H_

#include "content/browser/picture_in_picture/video_picture_in_picture_window_controller_impl.h"

#include "content/common/content_export.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {

class WebContents;

class CONTENT_EXPORT BraveVideoPictureInPictureWindowControllerImpl
    : public VideoPictureInPictureWindowControllerImpl {
 public:
  // Gets a reference to the controller associated with |web_contents| and
  // creates one if it does not exist. The returned pointer is guaranteed to be
  // non-null.
  static BraveVideoPictureInPictureWindowControllerImpl*
  GetOrCreateForWebContents(WebContents* web_contents);

  BraveVideoPictureInPictureWindowControllerImpl(
      const BraveVideoPictureInPictureWindowControllerImpl&) = delete;
  BraveVideoPictureInPictureWindowControllerImpl& operator=(
      const BraveVideoPictureInPictureWindowControllerImpl&) = delete;

  ~BraveVideoPictureInPictureWindowControllerImpl() override;

  // Use
  // BraveVideoPictureInPictureWindowControllerImpl::GetOrCreateForWebContents()
  // to create an instance.
  explicit BraveVideoPictureInPictureWindowControllerImpl(
      WebContents* web_contents);

 private:
};

}  // namespace content

#endif  // BRAVE_CONTENT_BROWSER_PICTURE_IN_PICTURE_BRAVE_VIDEO_PICTURE_IN_PICTURE_WINDOW_CONTROLLER_IMPL_H_
