/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <utility>

#include "base/task/sequenced_task_runner.h"
#include "brave/content/public/browser/devtools/adblock_devtools_instumentation.h"
#include "content/browser/devtools/devtools_agent_host_impl.h"
#include "content/browser/devtools/protocol/network_handler.h"
#include "content/browser/devtools/render_frame_devtools_agent_host.h"
#include "content/browser/renderer_host/frame_tree_node.h"
#include "content/browser/renderer_host/navigation_request.h"
#include "content/public/browser/browser_thread.h"

namespace {

void SendAdblockInfoInternal(
    content::FrameTreeNodeId frame_tree_node_id,
    const std::string& request_id,
    const content::devtools_instrumentation::AdblockInfo& info) {
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    content::GetUIThreadTaskRunner()->PostTask(
        FROM_HERE, base::BindOnce(&SendAdblockInfoInternal, frame_tree_node_id,
                                  request_id, info));
    return;
  }
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  content::FrameTreeNode* frame_tree_node =
      content::FrameTreeNode::GloballyFindByID(frame_tree_node_id);

  if (!frame_tree_node) {
    return;
  }
  content::DevToolsAgentHostImpl* agent_host =
      content::RenderFrameDevToolsAgentHost::GetFor(frame_tree_node);
  if (!agent_host) {
    return;
  }

  auto adblock_info =
      content::protocol::Network::AdblockInfo::Create()
          .SetRequestUrl(info.request_url.spec())
          .SetCheckedUrl(info.checked_url.spec())
          .SetSourceHost(info.source_host)
          .SetAggressive(info.aggressive)
          .SetResourceType(
              content::protocol::NetworkHandler::ResourceTypeToString(
                  info.resource_type.value()))
          .SetBlocked(info.blocked)
          .SetDidMatchImportantRule(info.did_match_important_rule)
          .SetDidMatchRule(info.did_match_rule)
          .SetDidMatchException(info.did_match_exception)
          .SetHasMockData(info.has_mock_data)
          .Build();

  if (info.rewritten_url) {
    adblock_info->SetRewrittenUrl(info.rewritten_url.value());
  }

  for (auto* handler :
       content::protocol::NetworkHandler::ForAgentHost(agent_host)) {
    handler->RequestAdblockInfoReceived(request_id, adblock_info->Clone());
  }
}

}  // namespace

namespace content::devtools_instrumentation {

AdblockInfo::AdblockInfo() = default;
AdblockInfo::~AdblockInfo() = default;
AdblockInfo::AdblockInfo(const AdblockInfo&) = default;
AdblockInfo::AdblockInfo(AdblockInfo&&) = default;
AdblockInfo& AdblockInfo::operator=(const AdblockInfo&) = default;
AdblockInfo& AdblockInfo::operator=(AdblockInfo&&) = default;

void SendAdblockInfo(content::FrameTreeNodeId frame_tree_node_id,
                     const std::string& request_id,
                     const AdblockInfo& info) {
  SendAdblockInfoInternal(frame_tree_node_id, request_id, info);
}

void SendAdblockInfo(content::NavigationHandle* handle,
                     const AdblockInfo& info) {
  content::NavigationRequest* request =
      content::NavigationRequest::From(handle);
  if (!request || !request->devtools_navigation_token()) {
    return;
  }
  SendAdblockInfoInternal(request->GetFrameTreeNodeId(),
                          request->devtools_navigation_token().ToString(),
                          info);
}

}  // namespace content::devtools_instrumentation
