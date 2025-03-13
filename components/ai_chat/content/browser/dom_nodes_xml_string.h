// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_DOM_NODES_XML_STRING_H_
#define BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_DOM_NODES_XML_STRING_H_

#include <string>

#include "ui/accessibility/ax_tree_update.h"

namespace ai_chat {

std::string GetDomNodesXmlString(ui::AXTreeUpdate& tree);

}  // namespace ai_chat

#endif  // BRAVE_COMPONENTS_AI_CHAT_CONTENT_BROWSER_DOM_NODES_XML_STRING_H_
