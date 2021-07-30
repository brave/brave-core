/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/net/network_delegate_helper.h"

#include <memory>
#include <string>

#include "base/task/post_task.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "components/sessions/content/session_tab_helper.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"
#include "url/gurl.h"

namespace brave_rewards {

namespace {

void DispatchOnUI(
    const std::string post_data,
    const GURL url,
    const GURL first_party_url,
    const std::string referrer,
    int frame_tree_node_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto* web_contents =
      content::WebContents::FromFrameTreeNodeId(frame_tree_node_id);
  if (!web_contents)
    return;

  auto* tab_helper = sessions::SessionTabHelper::FromWebContents(web_contents);
  if (!tab_helper)
    return;

  auto* rewards_service = RewardsServiceFactory::GetForProfile(
      Profile::FromBrowserContext(web_contents->GetBrowserContext()));
  if (rewards_service)
    rewards_service->OnPostData(tab_helper->session_id(),
                                url, first_party_url,
                                GURL(referrer), post_data);
}

}  // namespace

int OnBeforeURLRequest(
  const brave::ResponseCallback& next_callback,
  std::shared_ptr<brave::BraveRequestInfo> ctx) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (IsMediaLink(ctx->request_url, ctx->tab_origin, ctx->referrer)) {
    if (!ctx->upload_data.empty()) {
      DispatchOnUI(ctx->upload_data, ctx->request_url, ctx->tab_url,
                   ctx->referrer.spec(), ctx->frame_tree_node_id);
    }
  }

  return net::OK;
}

}  // namespace brave_rewards



