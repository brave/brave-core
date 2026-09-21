/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_DRAG_DROP_BRAVE_DRAG_DROP_IMAGE_METADATA_STRIPPER_H_
#define BRAVE_BROWSER_DRAG_DROP_BRAVE_DRAG_DROP_IMAGE_METADATA_STRIPPER_H_

#include "content/public/browser/web_contents_view_delegate.h"

namespace content {
class WebContents;
}  // namespace content

namespace brave {

// Wraps |callback|, the completion callback `HandleOnPerformingDrop` runs to
// hand the dropped files back to the view, so that flagged metadata is stripped
// out of dropped jpeg images. This is the last browser-side hook before
// `RenderWidgetHostImpl::DragTargetDrop` grants the renderer access to those
// paths.
//
// Returns |callback| unchanged when the feature is off, leaving the drop on its
// synchronous upstream path. Otherwise the returned callback completes the drop
// asynchronously, which the views and the mac drop paths both support.
//
// As with the upload flow, the dropped files themselves are never modified.
// Each strippable jpeg is copied into a unique subdirectory of one temporary
// parent directory (so the original basename is preserved) and
// `DropData::filenames` is pointed at the copy. The page holds those copies for
// as long as it holds the `File`, so the parent directory is owned by
// |web_contents| and recursively deleted once it is destroyed.
content::WebContentsViewDelegate::DropCompletionCallback
MaybeStripImageMetadataForDrop(
    content::WebContents* web_contents,
    content::WebContentsViewDelegate::DropCompletionCallback callback);

}  // namespace brave

#endif  // BRAVE_BROWSER_DRAG_DROP_BRAVE_DRAG_DROP_IMAGE_METADATA_STRIPPER_H_
