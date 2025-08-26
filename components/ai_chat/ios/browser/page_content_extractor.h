// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_PAGE_CONTENT_EXTRACTOR_H_
#define BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_PAGE_CONTENT_EXTRACTOR_H_

#include <cstdint>
#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/ai_chat/core/common/mojom/page_content_extractor.mojom.h"
#include "ios/web/public/js_messaging/web_frames_manager.h"
#include "ios/web/public/web_state_observer.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace base {
class TimeTicks;
class Value;
}  // namespace base

namespace mojo {
template <typename T>
class PendingReceiver;
}  // namespace mojo

namespace web {
class WebFrame;
class WebState;
}  // namespace web

namespace ai_chat {

class PageContentExtractor : public ai_chat::mojom::PageContentExtractor,
                             public web::WebStateObserver,
                             public web::WebFramesManager::Observer {
 public:
  PageContentExtractor(web::WebState* web_state);

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
      base::TimeTicks);
  void OnJSYoutubeInnerTubeConfigResult(
      ai_chat::mojom::PageContentExtractor::ExtractPageContentCallback callback,
      ai_chat::mojom::PageContentType type,
      std::optional<base::Value> value,
      base::TimeTicks);
  void OnDistillResult(
      mojom::PageContentExtractor::ExtractPageContentCallback callback,
      std::optional<base::Value> content,
      base::TimeTicks);

  // WebFramesManager::Observer implementation:
  void WebFrameBecameAvailable(web::WebFramesManager* web_frames_manager,
                               web::WebFrame* web_frame) override;
  void WebFrameBecameUnavailable(web::WebFramesManager* web_frames_manager,
                                 const std::string& frame_id) override;

  // WebStateObserver implementation:
  void WebStateDestroyed(web::WebState* web_state) override;

  void BindReceiver(
      mojo::PendingReceiver<mojom::PageContentExtractor> receiver);

  // Utils / Emulation
  void ExecuteJavaScript(const std::u16string& script,
                         base::OnceCallback<void(std::optional<base::Value>,
                                                 base::TimeTicks)> callback);

  raw_ptr<web::WebState> web_state_;
  raw_ptr<web::WebFrame> main_frame_;

  mojo::Receiver<mojom::PageContentExtractor> receiver_{this};

  base::WeakPtrFactory<PageContentExtractor> weak_ptr_factory_{this};
};

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_IOS_BROWSER_PAGE_CONTENT_EXTRACTOR_H_
