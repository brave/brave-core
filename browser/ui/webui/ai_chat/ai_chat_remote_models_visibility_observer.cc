// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/ai_chat/ai_chat_remote_models_visibility_observer.h"

#include "brave/components/ai_chat/core/browser/model_service.h"
#include "content/public/browser/visibility.h"
#include "content/public/browser/web_contents.h"

namespace ai_chat {

AIChatRemoteModelsVisibilityObserver::AIChatRemoteModelsVisibilityObserver(
    content::WebContents* web_contents,
    ModelService* model_service)
    : content::WebContentsObserver(web_contents),
      model_service_(model_service) {
  OnVisibilityChanged(web_contents->GetVisibility());
}

AIChatRemoteModelsVisibilityObserver::~AIChatRemoteModelsVisibilityObserver() {
  if (is_surface_visible_) {
    model_service_->OnRemoteModelsSurfaceHidden();
  }
}

void AIChatRemoteModelsVisibilityObserver::OnVisibilityChanged(
    content::Visibility visibility) {
  const bool is_now_visible = visibility == content::Visibility::VISIBLE;
  if (is_now_visible == is_surface_visible_) {
    return;
  }

  is_surface_visible_ = is_now_visible;
  if (is_surface_visible_) {
    model_service_->OnRemoteModelsSurfaceVisible();
  } else {
    model_service_->OnRemoteModelsSurfaceHidden();
  }
}

}  // namespace ai_chat
