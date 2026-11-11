/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/public/browser/web_contents_view_delegate.h"

namespace content {
class WebContents;
}  // namespace content

namespace brave {

// The drop-metadata strip is spliced into HandleOnPerformingDrop by
// rewrite/chrome/browser/ui/tab_contents/
// chrome_web_contents_view_handle_drop.cc.yaml.
// Defined in
// brave/browser/drag_drop/brave_drag_drop_image_metadata_stripper.cc, and
// declared here so that chrome/browser does not depend on Brave targets.
content::WebContentsViewDelegate::DropCompletionCallback
MaybeStripImageMetadataForDrop(
    content::WebContents* web_contents,
    content::WebContentsViewDelegate::DropCompletionCallback callback);

}  // namespace brave

#include <chrome/browser/ui/tab_contents/chrome_web_contents_view_handle_drop.cc>
