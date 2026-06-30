// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_PAGE_CONTENT_BLOCKS_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_PAGE_CONTENT_BLOCKS_H_

#include <vector>

#include "brave/components/ai_chat/core/common/mojom/common.mojom-forward.h"

namespace optimization_guide::proto {
class AnnotatedPageContent;
}  // namespace optimization_guide::proto

namespace ai_chat {

// Controls how much detail is serialized when converting page content.
enum class PageContentDetail {
  // Full detail for agentic tools that need to target and act on elements.
  // Includes everything kContentOnly keeps, plus the agentic targeting metadata
  // that the default extraction still emits: `dom_id`, scroll state
  // (`scrollable`/`size`/`visible_area`), document identifiers, the
  // viewport/scroll summary, and the interaction instructions.
  kAgentic,
  // Reduced detail for plain content extraction (e.g. AI Chat summarizing or
  // answering questions about a page). Drops the agentic targeting metadata
  // (`dom_id`, scroll state, document identifiers, the viewport/scroll summary,
  // and the interaction instructions), as well as SVG/canvas content and
  // generic container wrappers, none of which can be reliably used in this mode
  // and which only add noise and token cost. Interaction hints
  // (`clickable`/`editable`) and element geometry are kept when present, since
  // the default extraction rarely emits them.
  kContentOnly,
};

// Converts web content (in the form of AnnotatedPageContent) in to LLM-readable
// content (in the form of ContentBlocks), suitable for AI Tool responses.
// The conversion creates structured text that helps
// AI understand page content, target actions against specific elements and
// understand the state of the viewport.
//
// The output includes:
// - Hierarchical content structure
// - Form data and controls
// - Accessibility information
// And, when `detail` is `kAgentic`:
// - DOM IDs for targeting elements
// - Scroll data for the viewport and elements
std::vector<mojom::ContentBlockPtr> ConvertAnnotatedPageContentToBlocks(
    const optimization_guide::proto::AnnotatedPageContent& page_content,
    PageContentDetail detail = PageContentDetail::kAgentic);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_PAGE_CONTENT_BLOCKS_H_
