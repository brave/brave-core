/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_URL_CONTEXT_H_
#define BRAVE_BROWSER_NET_URL_CONTEXT_H_

#include <memory>
#include <set>
#include <string>

#include "net/url_request/referrer_policy.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "url/gurl.h"

class BraveRequestHandler;

namespace content {
class BrowserContext;
}

namespace network {
struct ResourceRequest;
}

namespace brave {
struct BraveRequestInfo;
using ResponseCallback = base::Callback<void()>;
}  // namespace brave

namespace brave_rewards {
int OnBeforeURLRequest(const brave::ResponseCallback& next_callback,
                       std::shared_ptr<brave::BraveRequestInfo> ctx);
}  // namespace brave_rewards

namespace brave {

enum BraveNetworkDelegateEventType {
  kOnBeforeRequest,
  kOnBeforeStartTransaction,
  kOnHeadersReceived,
  kOnCanGetCookies,
  kOnCanSetCookies,
  kUnknownEventType
};

enum BlockedBy { kNotBlocked, kAdBlocked, kOtherBlocked };

struct BraveRequestInfo {
  BraveRequestInfo();

  // For tests, should not be used directly.
  explicit BraveRequestInfo(const GURL& url);

  ~BraveRequestInfo();
  GURL request_url;
  GURL tab_origin;
  GURL tab_url;
  GURL initiator_url;

  GURL referrer;
  net::ReferrerPolicy referrer_policy =
      net::ReferrerPolicy::CLEAR_ON_TRANSITION_FROM_SECURE_TO_INSECURE;
  base::Optional<GURL> new_referrer;

  std::string new_url_spec;
  bool allow_brave_shields = true;
  bool allow_ads = false;
  bool allow_http_upgradable_resource = false;
  bool allow_referrers = false;
  bool is_webtorrent_disabled = false;
  int render_process_id = 0;
  int render_frame_id = 0;
  int frame_tree_node_id = 0;
  uint64_t request_identifier = 0;
  size_t next_url_request_index = 0;

  net::HttpRequestHeaders* headers = nullptr;
  // The following two sets are populated by |OnBeforeStartTransactionCallback|.
  // |set_headers| contains headers which values were added or modified.
  std::set<std::string> set_headers;
  std::set<std::string> removed_headers;
  const net::HttpResponseHeaders* original_response_headers = nullptr;
  scoped_refptr<net::HttpResponseHeaders>* override_response_headers = nullptr;

  GURL* allowed_unsafe_redirect_url = nullptr;
  BraveNetworkDelegateEventType event_type = kUnknownEventType;
  const base::ListValue* referral_headers_list = nullptr;
  BlockedBy blocked_by = kNotBlocked;
  bool cancel_request_explicitly = false;
  std::string mock_data_url;

  // Default to invalid type for resource_type, so delegate helpers
  // can properly detect that the info couldn't be obtained.
  // TODO(iefremov): Replace with something like |WebRequestResourceType| to
  // distinguish WebSockets.
  static constexpr blink::mojom::ResourceType kInvalidResourceType =
      static_cast<blink::mojom::ResourceType>(-1);
  blink::mojom::ResourceType resource_type = kInvalidResourceType;

  std::string upload_data;

  static void FillCTX(const network::ResourceRequest& request,
                      int render_process_id,
                      int frame_tree_node_id,
                      uint64_t request_identifier,
                      content::BrowserContext* browser_context,
                      std::shared_ptr<brave::BraveRequestInfo> ctx);

 private:
  // Please don't add any more friends here if it can be avoided.
  // We should also remove the one below.
  friend class ::BraveRequestHandler;

  GURL* new_url = nullptr;

  DISALLOW_COPY_AND_ASSIGN(BraveRequestInfo);
};

// ResponseListener
using OnBeforeURLRequestCallback =
    base::Callback<int(const ResponseCallback& next_callback,
                       std::shared_ptr<BraveRequestInfo> ctx)>;
using OnBeforeStartTransactionCallback =
    base::Callback<int(net::HttpRequestHeaders* headers,
                       const ResponseCallback& next_callback,
                       std::shared_ptr<BraveRequestInfo> ctx)>;
using OnHeadersReceivedCallback = base::Callback<int(
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url,
    const ResponseCallback& next_callback,
    std::shared_ptr<BraveRequestInfo> ctx)>;

}  // namespace brave

#endif  // BRAVE_BROWSER_NET_URL_CONTEXT_H_
