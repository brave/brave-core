// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/ai_chat/ai_chat_remote_models_visibility_observer.h"

#include "brave/components/ai_chat/core/browser/model_service.h"
#include "ios/web/public/web_state.h"

namespace ai_chat {

AIChatRemoteModelsVisibilityObserver::AIChatRemoteModelsVisibilityObserver(
    web::WebState* web_state,
    ModelService* model_service)
    : web_state_(web_state), model_service_(model_service) {
  web_state_->AddObserver(this);
  if (web_state_->IsVisible()) {
    is_surface_visible_ = true;
    model_service_->OnRemoteModelsSurfaceVisible();
  }
}

AIChatRemoteModelsVisibilityObserver::~AIChatRemoteModelsVisibilityObserver() {
  web_state_->RemoveObserver(this);
  if (is_surface_visible_) {
    model_service_->OnRemoteModelsSurfaceHidden();
  }
}

void AIChatRemoteModelsVisibilityObserver::WasShown(web::WebState* web_state) {
  if (is_surface_visible_) {
    return;
  }
  is_surface_visible_ = true;
  model_service_->OnRemoteModelsSurfaceVisible();
}

void AIChatRemoteModelsVisibilityObserver::WasHidden(web::WebState* web_state) {
  if (!is_surface_visible_) {
    return;
  }
  is_surface_visible_ = false;
  model_service_->OnRemoteModelsSurfaceHidden();
}

}  // namespace ai_chat
