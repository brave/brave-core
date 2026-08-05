// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_REMOTE_MODELS_VISIBILITY_OBSERVER_H_
#define BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_REMOTE_MODELS_VISIBILITY_OBSERVER_H_

#include "base/memory/raw_ptr.h"
#include "content/public/browser/web_contents_observer.h"

namespace content {
class WebContents;
enum class Visibility;
}  // namespace content

namespace ai_chat {

class ModelService;

// Forwards a Leo `WebContents`'s visibility transitions to
// `ModelService::OnRemoteModelsSurfaceVisible()`/`OnRemoteModelsSurfaceHidden()`,
// which gate the remote model list's refresh timer. Side-panel "switch to a
// different entry" caches the `WebContents` alive rather than destroying it
// (`SidePanelEntry::CacheView()`), so construction/destruction of the owning
// `AIChatUI` is not a reliable close signal — visibility is.
class AIChatRemoteModelsVisibilityObserver
    : public content::WebContentsObserver {
 public:
  AIChatRemoteModelsVisibilityObserver(content::WebContents* web_contents,
                                       ModelService* model_service);
  AIChatRemoteModelsVisibilityObserver(
      const AIChatRemoteModelsVisibilityObserver&) = delete;
  AIChatRemoteModelsVisibilityObserver& operator=(
      const AIChatRemoteModelsVisibilityObserver&) = delete;
  ~AIChatRemoteModelsVisibilityObserver() override;

 private:
  // content::WebContentsObserver:
  void OnVisibilityChanged(content::Visibility visibility) override;

  raw_ptr<ModelService> model_service_;
  bool is_surface_visible_ = false;
};

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_REMOTE_MODELS_VISIBILITY_OBSERVER_H_
