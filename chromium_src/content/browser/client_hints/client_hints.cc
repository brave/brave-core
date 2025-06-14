/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "content/browser/client_hints/client_hints.h"

#include "content/browser/renderer_host/frame_tree.h"
#include "content/browser/renderer_host/frame_tree_node.h"
#include "content/public/browser/client_hints_controller_delegate.h"
#include "content/public/browser/content_browser_client.h"
#include "brave/components/brave_user_agent/browser/brave_user_agent_exceptions.h"

namespace {
  GURL GetTopURL(content::FrameTreeNode* frame_tree_node) {
    return frame_tree_node->frame_tree().root()->current_url();
  }
}  // namespace

#define GetUserAgentMetadata() BraveGetUserAgentMetadata(GetTopURL(frame_tree_node))

#include "src/content/browser/client_hints/client_hints.cc"

#undef GetUserAgentMetadata
