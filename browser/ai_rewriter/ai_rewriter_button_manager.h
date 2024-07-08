#ifndef BRAVE_BROWSER_AI_REWRITER_AI_REWRITER_BUTTON_MANAGER_H_
#define BRAVE_BROWSER_AI_REWRITER_AI_REWRITER_BUTTON_MANAGER_H_
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/browser/ai_rewriter/ai_rewriter_button_model.h"
#include "brave/components/ai_rewriter/common/mojom/ai_rewriter.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "content/public/browser/render_frame_host.h"
#include "mojo/public/cpp/bindings/associated_receiver_set.h"
#include "mojo/public/cpp/bindings/pending_associated_receiver.h"
#include "ui/gfx/geometry/rect.h"

namespace ai_rewriter {

class AIRewriterButtonManager : public KeyedService,
                                public mojom::AIRewriterButton {
 public:
  AIRewriterButtonManager();
  AIRewriterButtonManager(const AIRewriterButtonManager&) = delete;
  AIRewriterButtonManager& operator=(const AIRewriterButtonManager&) = delete;
  ~AIRewriterButtonManager();

  static void Bind(
      content::RenderFrameHost* host,
      mojo::PendingAssociatedReceiver<mojom::AIRewriterButton> receiver);

  // mojom::AIRewriterButton:
  void Hide() override;
  void Show(const gfx::Rect& rect) override;

 private:
  mojo::AssociatedReceiverSet<mojom::AIRewriterButton,
                              base::WeakPtr<AIRewriterButtonModel>>
      receivers_;
};

}  // namespace ai_rewriter

#endif  // BRAVE_BROWSER_AI_REWRITER_AI_REWRITER_BUTTON_MANAGER_H_
