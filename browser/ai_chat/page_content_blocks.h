// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_AI_CHAT_PAGE_CONTENT_BLOCKS_H_
#define BRAVE_BROWSER_AI_CHAT_PAGE_CONTENT_BLOCKS_H_

#include <vector>

#include "brave/components/ai_chat/core/common/mojom/ai_chat.mojom-forward.h"
#include "components/optimization_guide/proto/features/common_quality_data.pb.h"

namespace ai_chat {

// Converts web content (in the form of AnnotatedPageContent) in to LLM-readable
// content (in the form of ContentBlocks), suitable for AI Tool responses.
// The conversion creates structured text that helps
// AI understand page content, target actions against specific elements and
// understand the state of the viewport.
//
// The output includes:
// - Summary of interactive elements with DOM IDs and coordinates
// - Hierarchical content structure
// - Form data and controls
// - Accessibility information
// - Scroll data for viewport and elements
std::vector<mojom::ContentBlockPtr> ConvertAnnotatedPageContentToBlocks(
    const optimization_guide::proto::AnnotatedPageContent& page_content);

}  // namespace ai_chat

#endif  // BRAVE_BROWSER_AI_CHAT_PAGE_CONTENT_BLOCKS_H_
