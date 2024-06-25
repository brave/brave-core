// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_REWRITER_RENDERER_AI_REWRITER_BUTTON_POSITIONER_H_
#define BRAVE_COMPONENTS_AI_REWRITER_RENDERER_AI_REWRITER_BUTTON_POSITIONER_H_

#include "base/component_export.h"
#include "base/functional/callback_helpers.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/ai_rewriter/common/mojom/ai_rewriter.mojom.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/associated_remote.h"

namespace content {
class RenderFrame;
}

namespace blink {
class WebDocument;
class WebDOMEvent;
}  // namespace blink

namespace ai_rewriter {

class COMPONENT_EXPORT(AI_REWRITER_RENDERER) AIRewriterButtonPositioner
    : public content::RenderFrameObserver {
 public:
  explicit AIRewriterButtonPositioner(content::RenderFrame* frame);
  AIRewriterButtonPositioner(const AIRewriterButtonPositioner&) = delete;
  AIRewriterButtonPositioner& operator=(const AIRewriterButtonPositioner&) =
      delete;
  ~AIRewriterButtonPositioner() override;

 protected:
  // content::RenderFrameObserver:
  void DidCreateDocumentElement() override;
  void OnDestruct() override;
  void DidChangeScrollOffset() override;

 private:
  void UpdateButton(blink::WebDocument document, blink::WebDOMEvent event);

  base::ScopedClosureRunner remove_listener_;
  mojo::AssociatedRemote<mojom::AIRewriterButton> button_;

  base::WeakPtrFactory<AIRewriterButtonPositioner> weak_ptr_factory_{this};
};

}  // namespace ai_rewriter

#endif  // BRAVE_COMPONENTS_AI_REWRITER_RENDERER_AI_REWRITER_BUTTON_POSITIONER_H_
