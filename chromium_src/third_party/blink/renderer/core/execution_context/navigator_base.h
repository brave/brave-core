/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_EXECUTION_CONTEXT_NAVIGATOR_BASE_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_EXECUTION_CONTEXT_NAVIGATOR_BASE_H_

#include "third_party/blink/public/platform/web_content_settings_client.h"
#include "third_party/blink/renderer/core/frame/local_dom_window.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/navigator_id.h"
#include "third_party/blink/renderer/core/loader/frame_loader.h"

#define BRAVE_NAVIGATOR_BASE_USER_AGENT                                \
  {                                                                         \
    brave_page_graph::PageGraph* page_graph =                               \
      DomWindow()->GetFrame()->GetDocument()->GetPageGraph();               \
    if (page_graph != nullptr) {                                            \
      String result = DomWindow()->GetFrame()->Loader().UserAgent();        \
      page_graph->RegisterWebAPICall("NavigatorID.userAgent",               \
        std::vector<const String>());                                       \
      page_graph->RegisterWebAPIResult("NavigatorID.userAgent", result);    \
    }                                                                       \
  }                                                                         \
  if (blink::WebContentSettingsClient* settings =                      \
          brave::GetContentSettingsClientFor(GetExecutionContext())) { \
    if (!settings->AllowFingerprinting(true)) {                        \
      return brave::BraveSessionCache::From(*(GetExecutionContext()))  \
          .FarbledUserAgent(GetExecutionContext()->UserAgent());       \
    }                                                                  \
  }

#include "../../../../../../../third_party/blink/renderer/core/execution_context/navigator_base.h"
#undef BRAVE_NAVIGATOR_BASE_USER_AGENT

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_RENDERER_CORE_EXECUTION_CONTEXT_NAVIGATOR_BASE_H_
