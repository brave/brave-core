/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/net/network_delegate_helper.h"

#include "base/task/post_task.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sessions/session_tab_helper.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/resource_request_info.h"
#include "content/public/browser/websocket_handshake_request_info.h"
#include "content/public/browser/web_contents.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/base/upload_data_stream.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

namespace brave_rewards {

namespace {

bool GetPostData(const net::URLRequest* request, std::string* post_data) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!request->has_upload())
    return false;

  const net::UploadDataStream* stream = request->get_upload();
  if (!stream->GetElementReaders())
    return false;

  const auto* element_readers = stream->GetElementReaders();

  if (element_readers->empty())
    return false;

  post_data->clear();
  for (const auto& element_reader : *element_readers) {
    const net::UploadBytesElementReader* reader =
        element_reader->AsBytesReader();
    if (!reader) {
      post_data->clear();
      return false;
    }
    post_data->append(reader->bytes(), reader->length());
  }
  return true;
}

void GetRenderFrameInfo(net::URLRequest* request,
                        int* render_frame_id,
                        int* render_process_id,
                        int* frame_tree_node_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  *render_frame_id = -1;
  *render_process_id = -1;
  *frame_tree_node_id = -1;

  // PlzNavigate requests have a frame_tree_node_id, but no render_process_id
  auto* request_info = content::ResourceRequestInfo::ForRequest(request);
  if (request_info) {
    *frame_tree_node_id = request_info->GetFrameTreeNodeId();
  }
  if (!content::ResourceRequestInfo::GetRenderFrameForRequest(
          request, render_process_id, render_frame_id)) {
    const content::WebSocketHandshakeRequestInfo* websocket_info =
      content::WebSocketHandshakeRequestInfo::ForRequest(request);
    if (websocket_info) {
      *render_frame_id = websocket_info->GetRenderFrameId();
      *render_process_id = websocket_info->GetChildId();
    }
  }
}

content::WebContents* GetWebContents(
    int render_process_id,
    int render_frame_id,
    int frame_tree_node_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  content::WebContents* web_contents =
      content::WebContents::FromFrameTreeNodeId(frame_tree_node_id);
  if (!web_contents) {
    content::RenderFrameHost* rfh =
        content::RenderFrameHost::FromID(render_process_id, render_frame_id);
    if (!rfh) {
      return nullptr;
    }
    web_contents =
        content::WebContents::FromRenderFrameHost(rfh);
  }
  return web_contents;
}

void DispatchOnUI(
    const std::string post_data,
    const GURL url,
    const GURL first_party_url,
    const std::string referrer,
    int render_process_id,
    int render_frame_id,
    int frame_tree_node_id) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  auto* web_contents = GetWebContents(render_process_id,
                                      render_frame_id,
                                      frame_tree_node_id);
  if (!web_contents)
    return;

  auto* tab_helper = SessionTabHelper::FromWebContents(web_contents);
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
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  if (IsMediaLink(ctx->request_url,
                  ctx->request->site_for_cookies(),
                  GURL(ctx->request->referrer()))) {
    std::string post_data;
    if (GetPostData(ctx->request, &post_data)) {
      int render_process_id, render_frame_id, frame_tree_node_id;
      GetRenderFrameInfo(ctx->request, &render_frame_id, &render_process_id,
          &frame_tree_node_id);
      base::PostTaskWithTraits(FROM_HERE, {content::BrowserThread::UI},
          base::BindOnce(&DispatchOnUI,
              post_data,
              ctx->request_url, ctx->request->site_for_cookies(), ctx->request->referrer(),
              render_process_id, render_frame_id, frame_tree_node_id));
    }
  }

  return net::OK;
}

}  // namespace brave_rewards



