// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ai_rewriter/renderer/ai_rewriter_agent.h"

#include <utility>

#include "base/functional/bind.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_element.h"
#include "third_party/blink/public/web/web_frame.h"
#include "third_party/blink/public/web/web_frame_widget.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "ui/gfx/geometry/rect_f.h"

namespace ai_rewriter {
namespace {
gfx::RectF GetBounds(content::RenderFrame* frame) {
  auto document = frame->GetWebFrame()->GetDocument();

  auto focused = document.FocusedElement();
  if (focused.IsNull()) {
    return gfx::RectF();
  }

  return gfx::RectF(frame->ConvertViewportToWindow(focused.BoundsInWidget()));
}
}  // namespace

AIRewriterAgent::AIRewriterAgent(content::RenderFrame* render_frame,
                                 service_manager::BinderRegistry* registry)
    : content::RenderFrameObserver(render_frame) {
  registry->AddInterface(base::BindRepeating(&AIRewriterAgent::BindReceiver,
                                             base::Unretained(this)));
}

AIRewriterAgent::~AIRewriterAgent() = default;

void AIRewriterAgent::OnDestruct() {
  delete this;
}

void AIRewriterAgent::GetFocusBounds(GetFocusBoundsCallback callback) {
  std::move(callback).Run(GetBounds(render_frame()));
}

void AIRewriterAgent::BindReceiver(
    mojo::PendingReceiver<mojom::AIRewriterAgent> receiver) {
  VLOG(1) << "Receiver reset";
  receiver_.reset();
  receiver_.Bind(std::move(receiver));
}

}  // namespace ai_rewriter
