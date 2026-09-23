// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include <utility>

#include "content/public/browser/file_select_listener.h"

#include <chrome/browser/ui/views/profiles/signin_view_controller_delegate_views.cc>

namespace brave {

// Returns true after forwarding an enabled local profile customization request
// from its primary frame to FileSelectHelper. On false, the caller must handle
// the listener. The Brave target owns the feature and file chooser
// dependencies.
bool MaybeRunProfileCustomizationFileChooser(
    content::WebContents* web_contents,
    content::RenderFrameHost* render_frame_host,
    scoped_refptr<content::FileSelectListener> listener,
    const blink::mojom::FileChooserParams& params);

}  // namespace brave

void SigninViewControllerDelegateViews::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    scoped_refptr<content::FileSelectListener> listener,
    const blink::mojom::FileChooserParams& params) {
  // Only the local profile image picker needs file selection in this modal.
  if (!brave::MaybeRunProfileCustomizationFileChooser(
          GetWebContents(), render_frame_host, listener, params)) {
    content::WebContentsDelegate::RunFileChooser(render_frame_host,
                                                 std::move(listener), params);
  }
}
