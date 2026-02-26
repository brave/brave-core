/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NET_URL_CONTEXT_H_
#define BRAVE_BROWSER_NET_URL_CONTEXT_H_

#include <memory>
#include <optional>
#include <set>
#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/sequence_checker.h"
#include "content/public/browser/global_routing_id.h"
#include "net/base/network_anonymization_key.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/url_request/referrer_policy.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom-shared.h"
#include "url/gurl.h"

template <template <typename> class T>
class BraveRequestHandler;

namespace content {
class BrowserContext;
}

namespace network {
struct ResourceRequest;
}

namespace brave {
class BraveRequestInfo;
using ResponseCallback = base::RepeatingCallback<void()>;
}  // namespace brave

namespace brave_rewards {
template <template <typename> class T>
int OnBeforeURLRequest(const brave::ResponseCallback& next_callback,
                       T<brave::BraveRequestInfo> ctx);
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

class BraveRequestInfo {
 public:
  BraveRequestInfo();
  BraveRequestInfo(const BraveRequestInfo&) = delete;
  BraveRequestInfo& operator=(const BraveRequestInfo&) = delete;

  // For tests, should not be used directly.
  explicit BraveRequestInfo(const GURL& url);

  ~BraveRequestInfo();

  // Accessors
  const std::string& method() const;
  void set_method(const std::string& value);

  const GURL& request_url() const;
  void set_request_url(const GURL& value);

  const GURL& tab_origin() const;
  void set_tab_origin(const GURL& value);

  const GURL& tab_url() const;
  void set_tab_url(const GURL& value);

  const GURL& initiator_url() const;
  void set_initiator_url(const GURL& value);

  bool internal_redirect() const;
  void set_internal_redirect(bool value);

  const GURL& redirect_source() const;
  void set_redirect_source(const GURL& value);

  const GURL& referrer() const;
  void set_referrer(const GURL& value);

  net::ReferrerPolicy referrer_policy() const;
  void set_referrer_policy(net::ReferrerPolicy value);

  const std::optional<GURL>& new_referrer() const;
  void set_new_referrer(const std::optional<GURL>& value);

  const std::optional<int>& pending_error() const;
  void set_pending_error(const std::optional<int>& value);

  const std::string& new_url_spec() const;
  void set_new_url_spec(const std::string& value);

  bool allow_brave_shields() const;
  void set_allow_brave_shields(bool value);

  bool allow_ads() const;
  void set_allow_ads(bool value);

  bool aggressive_blocking() const;
  void set_aggressive_blocking(bool value);

  bool allow_http_upgradable_resource() const;
  void set_allow_http_upgradable_resource(bool value);

  bool allow_referrers() const;
  void set_allow_referrers(bool value);

  const content::GlobalRenderFrameHostToken& render_frame_token() const;
  void set_render_frame_token(const content::GlobalRenderFrameHostToken& value);

  uint64_t request_identifier() const;
  void set_request_identifier(uint64_t value);

  size_t next_url_request_index() const;
  void set_next_url_request_index(size_t value);

  content::BrowserContext* browser_context() const;
  void set_browser_context(content::BrowserContext* value);

  net::HttpRequestHeaders* headers() const;
  void set_headers(net::HttpRequestHeaders* value);

  const std::set<std::string>& modified_headers() const;
  std::set<std::string>& mutable_modified_headers();

  const std::set<std::string>& removed_headers() const;
  std::set<std::string>& mutable_removed_headers();

  const net::HttpResponseHeaders* original_response_headers() const;
  void set_original_response_headers(const net::HttpResponseHeaders* value);

  scoped_refptr<net::HttpResponseHeaders>* override_response_headers() const;
  void set_override_response_headers(
      scoped_refptr<net::HttpResponseHeaders>* value);

  GURL* allowed_unsafe_redirect_url() const;
  void set_allowed_unsafe_redirect_url(GURL* value);

  BraveNetworkDelegateEventType event_type() const;
  void set_event_type(BraveNetworkDelegateEventType value);

  BlockedBy blocked_by() const;
  void set_blocked_by(BlockedBy value);

  const std::string& mock_data_url() const;
  void set_mock_data_url(const std::string& value);

  bool ShouldMockRequest() const;

  const net::NetworkAnonymizationKey& network_anonymization_key() const;
  void set_network_anonymization_key(const net::NetworkAnonymizationKey& value);

  // Default to invalid type for resource_type, so delegate helpers
  // can properly detect that the info couldn't be obtained.
  // TODO(iefremov): Replace with something like |WebRequestResourceType| to
  // distinguish WebSockets.
  static constexpr blink::mojom::ResourceType kInvalidResourceType =
      static_cast<blink::mojom::ResourceType>(-1);

  blink::mojom::ResourceType resource_type() const;
  void set_resource_type(blink::mojom::ResourceType value);

  const std::string& upload_data() const;
  void set_upload_data(const std::string& value);

  const std::optional<std::string>& devtools_request_id() const;
  void set_devtools_request_id(const std::optional<std::string>& value);

  static std::unique_ptr<brave::BraveRequestInfo> MakeCTX(
      const network::ResourceRequest& request,
      content::GlobalRenderFrameHostToken render_frame_token,
      uint64_t request_identifier,
      content::BrowserContext* browser_context,
      brave::BraveRequestInfo* old_ctx);

  base::WeakPtr<BraveRequestInfo> AsWeakPtr() {
    return weak_factory_.GetWeakPtr();
  }

 private:
  // Please don't add any more friends here if it can be avoided.
  // We should also remove the one below.
  template <template <typename> class T>
  friend class ::BraveRequestHandler;

  GURL* new_url() const;
  void set_new_url(GURL* value);

  std::string method_;
  GURL request_url_;
  GURL tab_origin_;
  GURL tab_url_;
  GURL initiator_url_;

  bool internal_redirect_ = false;
  GURL redirect_source_;

  GURL referrer_;
  net::ReferrerPolicy referrer_policy_ =
      net::ReferrerPolicy::CLEAR_ON_TRANSITION_FROM_SECURE_TO_INSECURE;
  std::optional<GURL> new_referrer_;

  std::optional<int> pending_error_;
  std::string new_url_spec_;
  // TODO(iefremov): rename to shields_up.
  bool allow_brave_shields_ = true;
  bool allow_ads_ = false;
  // Whether or not Shields "aggressive" mode was enabled where the request was
  // initiated.
  bool aggressive_blocking_ = false;
  bool allow_http_upgradable_resource_ = false;
  bool allow_referrers_ = false;
  // GlobalRenderFrameHostToken: Uniquely identifies a RenderFrameHost instance.
  // FrameTreeNodeId: Identifies a persistent location in the frame tree
  // (remains constant across navigations) and is the primary handle for frame
  // tree manipulation.
  content::GlobalRenderFrameHostToken render_frame_token_;
  uint64_t request_identifier_ = 0;
  size_t next_url_request_index_ = 0;

  raw_ptr<content::BrowserContext, DanglingUntriaged> browser_context_ =
      nullptr;
  raw_ptr<net::HttpRequestHeaders> headers_ = nullptr;
  // The following two sets are populated by |OnBeforeStartTransactionCallback|.
  // |modified_headers| contains headers which values were added or modified.
  std::set<std::string> modified_headers_;
  std::set<std::string> removed_headers_;
  raw_ptr<const net::HttpResponseHeaders, DanglingUntriaged>
      original_response_headers_ = nullptr;
  raw_ptr<scoped_refptr<net::HttpResponseHeaders>, DanglingUntriaged>
      override_response_headers_ = nullptr;

  raw_ptr<GURL, DanglingUntriaged> allowed_unsafe_redirect_url_ = nullptr;
  BraveNetworkDelegateEventType event_type_ = kUnknownEventType;
  BlockedBy blocked_by_ = kNotBlocked;
  std::string mock_data_url_;

  net::NetworkAnonymizationKey network_anonymization_key_ =
      net::NetworkAnonymizationKey();

  blink::mojom::ResourceType resource_type_ = kInvalidResourceType;

  std::string upload_data_;

  std::optional<std::string> devtools_request_id_;

  raw_ptr<GURL, DanglingUntriaged> new_url_ = nullptr;

  SEQUENCE_CHECKER(sequence_checker_);
  base::WeakPtrFactory<BraveRequestInfo> weak_factory_{this};
};

// ResponseListener
template <template <typename> class T>
using OnBeforeURLRequestCallback =
    base::RepeatingCallback<int(const ResponseCallback& next_callback,
                                T<BraveRequestInfo> ctx)>;

template <template <typename> class T>
using OnBeforeStartTransactionCallback =
    base::RepeatingCallback<int(net::HttpRequestHeaders* headers,
                                const ResponseCallback& next_callback,
                                T<BraveRequestInfo> ctx)>;

template <template <typename> class T>
using OnHeadersReceivedCallback = base::RepeatingCallback<int(
    const net::HttpResponseHeaders* original_response_headers,
    scoped_refptr<net::HttpResponseHeaders>* override_response_headers,
    GURL* allowed_unsafe_redirect_url,
    const ResponseCallback& next_callback,
    T<BraveRequestInfo> ctx)>;

}  // namespace brave

#endif  // BRAVE_BROWSER_NET_URL_CONTEXT_H_
