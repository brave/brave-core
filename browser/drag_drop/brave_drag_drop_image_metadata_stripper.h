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

content::WebContentsViewDelegate::DropCompletionCallback
MaybeStripImageMetadataForDrop(
    content::WebContents* web_contents,
    content::WebContentsViewDelegate::DropCompletionCallback callback);

}  // namespace brave

#endif  // BRAVE_BROWSER_DRAG_DROP_BRAVE_DRAG_DROP_IMAGE_METADATA_STRIPPER_H_
