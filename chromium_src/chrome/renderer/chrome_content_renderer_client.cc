/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/ai_chat/renderer/page_content_extractor.h"
#include "brave/components/ai_rewriter/common/buildflags/buildflags.h"
#include "brave/components/content_settings/renderer/brave_content_settings_agent_impl.h"
#include "chrome/common/chrome_isolated_world_ids.h"
#include "chrome/renderer/process_state.h"
#include "components/dom_distiller/content/renderer/distillability_agent.h"
#include "components/feed/content/renderer/rss_link_reader.h"
#include "content/public/common/isolated_world_ids.h"

#if BUILDFLAG(ENABLE_AI_REWRITER)
#include "brave/components/ai_rewriter/common/features.h"
#include "brave/components/ai_rewriter/renderer/ai_rewriter_agent.h"
#endif

namespace {

void RenderFrameWithBinderRegistryCreated(
    content::RenderFrame* render_frame,
    service_manager::BinderRegistry* registry) {
  new feed::RssLinkReader(render_frame, registry);
  if (ai_chat::features::IsAIChatEnabled() && !IsIncognitoProcess()) {
    new ai_chat::PageContentExtractor(render_frame, registry,
                                      content::ISOLATED_WORLD_ID_GLOBAL,
                                      ISOLATED_WORLD_ID_BRAVE_INTERNAL);
  }

#if BUILDFLAG(ENABLE_AI_REWRITER)
  if (ai_rewriter::features::IsAIRewriterEnabled()) {
    new ai_rewriter::AIRewriterAgent(render_frame, registry);
  }
#endif
}

}  // namespace

// We need to do this here rather than in |BraveContentRendererClient| because
// some classes need access to the registry on ChromeRenderFrameObserver.
#define BRAVE_RENDER_FRAME_CREATED \
  RenderFrameWithBinderRegistryCreated(render_frame, registry);

// Prevents unnecessary js console logs spam.
#define DistillabilityAgent(render_frame, dcheck_is_on) \
  DistillabilityAgent(render_frame, false)

#include "src/chrome/renderer/chrome_content_renderer_client.cc"

#undef BRAVE_RENDER_FRAME_CREATED
#undef DistillabilityAgent
