#ifndef BRAVE_COMPONENTS_BRAVE_TALK_RENDERER_BRAVE_TALK_RENDER_FRAME_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_TALK_RENDERER_BRAVE_TALK_RENDER_FRAME_OBSERVER_H_

#include <memory>

#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "third_party/blink/public/web/web_navigation_type.h"
#include "v8/include/v8.h"

namespace brave_talk {

class BraveTalkRenderFrameObserver : public content::RenderFrameObserver {
 public:
  explicit BraveTalkRenderFrameObserver(content::RenderFrame* render_frame,
                                          int32_t world_id);
  BraveTalkRenderFrameObserver(const BraveTalkRenderFrameObserver&) =
      delete;
  BraveTalkRenderFrameObserver& operator=(
      const BraveTalkRenderFrameObserver&) = delete;
  ~BraveTalkRenderFrameObserver() override;

  // RenderFrameObserver implementation.
  void DidCreateScriptContext(v8::Local<v8::Context> context,
                              int32_t world_id) override;

 private:
  // RenderFrameObserver implementation.
  void OnDestruct() override;

  // Handle to "handler" JavaScript object functionality.
  std::unique_ptr<BraveTalkRenderFrameObserver> native_javascript_handle_;
  int32_t world_id_;
};

}  // namespace brave_search

#endif // BRAVE_COMPONENTS_BRAVE_TALK_RENDERER_BRAVE_TALK_RENDER_FRAME_OBSERVER_H_
