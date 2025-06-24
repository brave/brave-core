// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_RENDERER_PAGE_CONTENT_EXTRACTOR_H_
#define BRAVE_COMPONENTS_AI_CHAT_RENDERER_PAGE_CONTENT_EXTRACTOR_H_

#include <cstdint>
#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_observer_tracker.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "services/service_manager/public/cpp/binder_registry.h"

namespace base {
class TimeTicks;
class Value;
}  // namespace base
namespace content {
class RenderFrame;
}  // namespace content
namespace mojo {
template <typename T>
class PendingReceiver;
}  // namespace mojo

namespace ai_chat {

class PageContentExtractor
    : public ai_chat::mojom::PageContentExtractor,
      public content::RenderFrameObserver,
      public content::RenderFrameObserverTracker<PageContentExtractor> {
 public:
  PageContentExtractor(content::RenderFrame* render_frame,
                       service_manager::BinderRegistry* registry,
                       int32_t global_world_id,
                       int32_t isolated_world_id);

  PageContentExtractor(const PageContentExtractor&) = delete;
  PageContentExtractor& operator=(const PageContentExtractor&) = delete;
  ~PageContentExtractor() override;

  // PageContentExtractor implementation:
  void ExtractPageContent(
      mojom::PageContentExtractor::ExtractPageContentCallback callback)
      override;
  void GetSearchSummarizerKey(
      mojom::PageContentExtractor::GetSearchSummarizerKeyCallback callback)
      override;
  void GetOpenAIChatButtonNonce(
      mojom::PageContentExtractor::GetOpenAIChatButtonNonceCallback callback)
      override;

  base::WeakPtr<PageContentExtractor> GetWeakPtr();

 private:
  void OnJSTranscriptUrlResult(
      mojom::PageContentExtractor::ExtractPageContentCallback callback,
      ai_chat::mojom::PageContentType type,
      std::optional<base::Value> value,
      base::TimeTicks start_time);
  void OnJSYoutubeInnerTubeConfigResult(
      ai_chat::mojom::PageContentExtractor::ExtractPageContentCallback callback,
      ai_chat::mojom::PageContentType type,
      std::optional<base::Value> value,
      base::TimeTicks start_time);
  void OnDistillResult(
      mojom::PageContentExtractor::ExtractPageContentCallback callback,
      const std::optional<std::string>& content);

  // RenderFrameObserver implementation:
  void OnDestruct() override;

  void BindReceiver(
      mojo::PendingReceiver<mojom::PageContentExtractor> receiver);

  mojo::Receiver<mojom::PageContentExtractor> receiver_{this};

  int32_t global_world_id_;
  int32_t isolated_world_id_;

  base::WeakPtrFactory<PageContentExtractor> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_RENDERER_PAGE_CONTENT_EXTRACTOR_H_
