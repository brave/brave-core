// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_RENDERER_PAGE_CONTENT_EXTRACTOR_H_
#define BRAVE_COMPONENTS_AI_CHAT_RENDERER_PAGE_CONTENT_EXTRACTOR_H_

#include <cstdint>
#include <string>

#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ai_chat {

class PageContentExtractor : public ai_chat::mojom::PageContentExtractor,
                             public content::RenderFrameObserver {
 public:
  explicit PageContentExtractor(content::RenderFrame* render_frame,
                                service_manager::BinderRegistry* registry,
                                int32_t global_world_id,
                                int32_t isolated_world_id);
  PageContentExtractor(const PageContentExtractor&) = delete;
  PageContentExtractor& operator=(const PageContentExtractor&) = delete;
  ~PageContentExtractor() override;

 private:
  void OnJSTranscriptUrlResult(
      mojom::PageContentExtractor::ExtractPageContentCallback callback,
      ai_chat::mojom::PageContentType type,
      absl::optional<base::Value> value,
      base::TimeTicks start_time);
  void OnDistillResult(
      mojom::PageContentExtractor::ExtractPageContentCallback callback,
      const absl::optional<std::string>& content);

  // RenderFrameObserver implementation:
  void OnDestruct() override;

  // PageContentExtractor implementation:
  void ExtractPageContent(
      mojom::PageContentExtractor::ExtractPageContentCallback callback)
      override;

  void BindReceiver(
      mojo::PendingReceiver<mojom::PageContentExtractor> receiver);

  mojo::Receiver<mojom::PageContentExtractor> receiver_{this};

  int32_t global_world_id_;
  int32_t isolated_world_id_;

  base::WeakPtrFactory<PageContentExtractor> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_RENDERER_PAGE_CONTENT_EXTRACTOR_H_
