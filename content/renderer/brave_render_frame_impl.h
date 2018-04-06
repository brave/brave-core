/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CONTENT_RENDERER_BRAVE_RENDER_FRAME_IMPL_H_
#define BRAVE_CONTENT_RENDERER_BRAVE_RENDER_FRAME_IMPL_H_

#include "content/renderer/render_frame_impl.h"

namespace content {

class CONTENT_EXPORT BraveRenderFrameImpl
    : public RenderFrameImpl {
 public:
  explicit BraveRenderFrameImpl(CreateParams params);

 protected:
  void WillSendRequest(blink::WebURLRequest& request) override;
  void ApplyReferrerBlocking(blink::WebURLRequest& request);
  DISALLOW_COPY_AND_ASSIGN(BraveRenderFrameImpl);
};

}  // namespace content

#endif  // BRAVE_CONTENT_RENDERER_BRAVE_RENDER_FRAME_IMPL_H_
