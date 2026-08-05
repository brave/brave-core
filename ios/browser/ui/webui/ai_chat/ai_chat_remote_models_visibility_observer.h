// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_REMOTE_MODELS_VISIBILITY_OBSERVER_H_
#define BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_REMOTE_MODELS_VISIBILITY_OBSERVER_H_

#include "base/memory/raw_ptr.h"
#include "ios/web/public/web_state_observer.h"

namespace web {
class WebState;
}  // namespace web

namespace ai_chat {

class ModelService;

// Forwards a Leo `WebState`'s visibility transitions to
// `ModelService::OnRemoteModelsSurfaceVisible()`/`OnRemoteModelsSurfaceHidden()`,
// which gate the remote model list's refresh timer. Mirrors the desktop
// `AIChatRemoteModelsVisibilityObserver` (`content::WebContentsObserver`
// equivalent).
class AIChatRemoteModelsVisibilityObserver : public web::WebStateObserver {
 public:
  AIChatRemoteModelsVisibilityObserver(web::WebState* web_state,
                                       ModelService* model_service);
  AIChatRemoteModelsVisibilityObserver(
      const AIChatRemoteModelsVisibilityObserver&) = delete;
  AIChatRemoteModelsVisibilityObserver& operator=(
      const AIChatRemoteModelsVisibilityObserver&) = delete;
  ~AIChatRemoteModelsVisibilityObserver() override;

 private:
  // web::WebStateObserver:
  void WasShown(web::WebState* web_state) override;
  void WasHidden(web::WebState* web_state) override;

  raw_ptr<web::WebState> web_state_;
  raw_ptr<ModelService> model_service_;
  bool is_surface_visible_ = false;
};

}  // namespace ai_chat

#endif  // BRAVE_IOS_BROWSER_UI_WEBUI_AI_CHAT_AI_CHAT_REMOTE_MODELS_VISIBILITY_OBSERVER_H_
