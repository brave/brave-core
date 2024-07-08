// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ai_rewriter/ai_rewriter_button_manager.h"

#include "brave/browser/ai_rewriter/ai_rewriter_button_manager_factory.h"
#include "brave/browser/ai_rewriter/ai_rewriter_button_model.h"

namespace ai_rewriter {

AIRewriterButtonManager::AIRewriterButtonManager() = default;
AIRewriterButtonManager::~AIRewriterButtonManager() = default;

// static
void AIRewriterButtonManager::Bind(
    content::RenderFrameHost* host,
    mojo::PendingAssociatedReceiver<mojom::AIRewriterButton> receiver) {
  auto* context = host->GetBrowserContext();
  auto* service = AIRewriterButtonManagerFactory::GetForContext(context);
  if (!service) {
    return;
  }

  service.
}

}  // namespace ai_rewriter
