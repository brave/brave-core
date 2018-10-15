/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_URL_CONTEXT_
#define BRAVE_BROWSER_NET_URL_CONTEXT_


#include <string>

#include "chrome/browser/net/chrome_network_delegate.h"
#include "content/public/common/resource_type.h"
#include "net/url_request/url_request.h"
#include "url/gurl.h"

namespace brave {

struct BraveRequestInfo;
using ResponseCallback = base::Callback<void()>;

}  // namespace brave

namespace brave_rewards {
  int OnBeforeURLRequest(
      GURL* new_url,
      const brave::ResponseCallback& next_callback,
      std::shared_ptr<brave::BraveRequestInfo> ctx);
}  // namespace brave_rewards

namespace brave {

enum BraveNetworkDelegateEventType {
  kOnBeforeRequest,
  kOnBeforeStartTransaction,
  kOnHeadersReceived,
  kUnknownEventType
};

struct BraveRequestInfo {
  BraveRequestInfo();
  ~BraveRequestInfo();
  GURL request_url;
  GURL tab_origin;
  std::string new_url_spec;
  bool allow_brave_shields = true;
  bool allow_ads = false;
  bool allow_http_upgradable_resource = false;
  int render_process_id = 0;
  int render_frame_id = 0;
  int frame_tree_node_id = 0;
  uint64_t request_identifier = 0;
  size_t next_url_request_index = 0;
  net::HttpRequestHeaders* headers = nullptr;
  const net::HttpResponseHeaders* original_response_headers = nullptr;
  scoped_refptr<net::HttpResponseHeaders>* override_response_headers = nullptr;
  GURL* allowed_unsafe_redirect_url = nullptr;
  BraveNetworkDelegateEventType event_type = kUnknownEventType;
  const base::ListValue* referral_headers_list = nullptr;
  // Default to invalid type for resource_type, so delegate helpers
  // can properly detect that the info couldn't be obtained.
  content::ResourceType resource_type = content::RESOURCE_TYPE_LAST_TYPE;

  static void FillCTXFromRequest(net::URLRequest* request,
    std::shared_ptr<brave::BraveRequestInfo> ctx);

 private:
  // Please don't add any more friends here if it can be avoided.
  // We should also remove the ones below.
  friend int OnBeforeURLRequest_SiteHacksWork(
      GURL* new_url,
      const ResponseCallback& next_callback,
      std::shared_ptr<BraveRequestInfo> ctx);
  friend int brave_rewards::OnBeforeURLRequest(
      GURL* new_url,
      const brave::ResponseCallback& next_callback,
      std::shared_ptr<brave::BraveRequestInfo> ctx);
  friend int OnBeforeURLRequest_TorWork(
      GURL* new_url,
      const ResponseCallback& next_callback,
      std::shared_ptr<BraveRequestInfo> ctx);
  // Don't use this directly after any dispatch
  // request is deprecated, do not use it.
  net::URLRequest* request;

  DISALLOW_COPY_AND_ASSIGN(BraveRequestInfo);
};

//ResponseListener
using OnBeforeURLRequestCallback =
    base::Callback<int(GURL* new_url,
        const ResponseCallback& next_callback,
        std::shared_ptr<BraveRequestInfo> ctx)>;
using OnBeforeStartTransactionCallback =
    base::Callback<int(net::URLRequest* request,
        net::HttpRequestHeaders* headers,
        const ResponseCallback& next_callback,
        std::shared_ptr<BraveRequestInfo> ctx)>;
using OnHeadersReceivedCallback =
    base::Callback<int(net::URLRequest* request,
        const net::HttpResponseHeaders* original_response_headers,
        scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
        GURL* allowed_unsafe_redirect_url,
        const ResponseCallback& next_callback,
        std::shared_ptr<BraveRequestInfo> ctx)>;

}  // namespace brave


#endif  // BRAVE_BROWSER_NET_URL_CONTEXT_
