/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/url_context.h"

#include <memory>
#include <string>

#include "brave/browser/brave_shields/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/render_frame_host.h"
#include "net/base/isolation_info.h"
#include "services/network/public/cpp/resource_request.h"
#include "url/origin.h"

namespace brave {

namespace {

std::string GetUploadData(const network::ResourceRequest& request) {
  std::string upload_data;
  if (!request.request_body) {
    return {};
  }
  const auto* elements = request.request_body->elements();
  for (const network::DataElement& element : *elements) {
    if (element.type() == network::mojom::DataElementDataView::Tag::kBytes) {
      const auto& bytes = element.As<network::DataElementBytes>().bytes();
      upload_data.append(bytes.begin(), bytes.end());
    }
  }

  return upload_data;
}

}  // namespace

BraveRequestInfo::BraveRequestInfo() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

BraveRequestInfo::BraveRequestInfo(const GURL& url) : request_url_(url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

BraveRequestInfo::~BraveRequestInfo() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

// Getters and setters with sequence checking
const std::string& BraveRequestInfo::method() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return method_;
}

void BraveRequestInfo::set_method(const std::string& value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  method_ = value;
}

const GURL& BraveRequestInfo::request_url() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return request_url_;
}

void BraveRequestInfo::set_request_url(const GURL& value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  request_url_ = value;
}

const GURL& BraveRequestInfo::tab_origin() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return tab_origin_;
}

void BraveRequestInfo::set_tab_origin(const GURL& value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  tab_origin_ = value;
}

const GURL& BraveRequestInfo::tab_url() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return tab_url_;
}

void BraveRequestInfo::set_tab_url(const GURL& value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  tab_url_ = value;
}

const GURL& BraveRequestInfo::initiator_url() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return initiator_url_;
}

void BraveRequestInfo::set_initiator_url(const GURL& value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  initiator_url_ = value;
}

bool BraveRequestInfo::internal_redirect() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return internal_redirect_;
}

void BraveRequestInfo::set_internal_redirect(bool value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  internal_redirect_ = value;
}

const GURL& BraveRequestInfo::redirect_source() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return redirect_source_;
}

void BraveRequestInfo::set_redirect_source(const GURL& value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  redirect_source_ = value;
}

const GURL& BraveRequestInfo::referrer() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return referrer_;
}

void BraveRequestInfo::set_referrer(const GURL& value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  referrer_ = value;
}

net::ReferrerPolicy BraveRequestInfo::referrer_policy() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return referrer_policy_;
}

void BraveRequestInfo::set_referrer_policy(net::ReferrerPolicy value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  referrer_policy_ = value;
}

const std::optional<GURL>& BraveRequestInfo::new_referrer() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return new_referrer_;
}

void BraveRequestInfo::set_new_referrer(const std::optional<GURL>& value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  new_referrer_ = value;
}

const std::optional<int>& BraveRequestInfo::pending_error() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return pending_error_;
}

void BraveRequestInfo::set_pending_error(const std::optional<int>& value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  pending_error_ = value;
}

const std::string& BraveRequestInfo::new_url_spec() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return new_url_spec_;
}

void BraveRequestInfo::set_new_url_spec(const std::string& value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  new_url_spec_ = value;
}

bool BraveRequestInfo::allow_brave_shields() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return allow_brave_shields_;
}

void BraveRequestInfo::set_allow_brave_shields(bool value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  allow_brave_shields_ = value;
}

bool BraveRequestInfo::allow_ads() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return allow_ads_;
}

void BraveRequestInfo::set_allow_ads(bool value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  allow_ads_ = value;
}

bool BraveRequestInfo::aggressive_blocking() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return aggressive_blocking_;
}

void BraveRequestInfo::set_aggressive_blocking(bool value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  aggressive_blocking_ = value;
}

bool BraveRequestInfo::allow_http_upgradable_resource() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return allow_http_upgradable_resource_;
}

void BraveRequestInfo::set_allow_http_upgradable_resource(bool value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  allow_http_upgradable_resource_ = value;
}

bool BraveRequestInfo::allow_referrers() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return allow_referrers_;
}

void BraveRequestInfo::set_allow_referrers(bool value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  allow_referrers_ = value;
}

const content::GlobalRenderFrameHostToken&
BraveRequestInfo::render_frame_token() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return render_frame_token_;
}

void BraveRequestInfo::set_render_frame_token(
    const content::GlobalRenderFrameHostToken& value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  render_frame_token_ = value;
}

uint64_t BraveRequestInfo::request_identifier() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return request_identifier_;
}

void BraveRequestInfo::set_request_identifier(uint64_t value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  request_identifier_ = value;
}

size_t BraveRequestInfo::next_url_request_index() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return next_url_request_index_;
}

void BraveRequestInfo::set_next_url_request_index(size_t value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  next_url_request_index_ = value;
}

content::BrowserContext* BraveRequestInfo::browser_context() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return browser_context_;
}

void BraveRequestInfo::set_browser_context(content::BrowserContext* value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  browser_context_ = value;
}

net::HttpRequestHeaders* BraveRequestInfo::headers() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return headers_;
}

void BraveRequestInfo::set_headers(net::HttpRequestHeaders* value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  headers_ = value;
}

const std::set<std::string>& BraveRequestInfo::modified_headers() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return modified_headers_;
}

std::set<std::string>& BraveRequestInfo::mutable_modified_headers() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return modified_headers_;
}

const std::set<std::string>& BraveRequestInfo::removed_headers() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return removed_headers_;
}

std::set<std::string>& BraveRequestInfo::mutable_removed_headers() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return removed_headers_;
}

const net::HttpResponseHeaders* BraveRequestInfo::original_response_headers()
    const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return original_response_headers_;
}

void BraveRequestInfo::set_original_response_headers(
    const net::HttpResponseHeaders* value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  original_response_headers_ = value;
}

scoped_refptr<net::HttpResponseHeaders>*
BraveRequestInfo::override_response_headers() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return override_response_headers_;
}

void BraveRequestInfo::set_override_response_headers(
    scoped_refptr<net::HttpResponseHeaders>* value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  override_response_headers_ = value;
}

GURL* BraveRequestInfo::allowed_unsafe_redirect_url() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return allowed_unsafe_redirect_url_;
}

void BraveRequestInfo::set_allowed_unsafe_redirect_url(GURL* value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  allowed_unsafe_redirect_url_ = value;
}

BraveNetworkDelegateEventType BraveRequestInfo::event_type() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return event_type_;
}

void BraveRequestInfo::set_event_type(BraveNetworkDelegateEventType value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  event_type_ = value;
}

BlockedBy BraveRequestInfo::blocked_by() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return blocked_by_;
}

void BraveRequestInfo::set_blocked_by(BlockedBy value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  blocked_by_ = value;
}

const std::string& BraveRequestInfo::mock_data_url() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return mock_data_url_;
}

void BraveRequestInfo::set_mock_data_url(const std::string& value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  mock_data_url_ = value;
}

bool BraveRequestInfo::ShouldMockRequest() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return blocked_by_ == kAdBlocked && !mock_data_url_.empty();
}

const net::NetworkAnonymizationKey&
BraveRequestInfo::network_anonymization_key() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return network_anonymization_key_;
}

void BraveRequestInfo::set_network_anonymization_key(
    const net::NetworkAnonymizationKey& value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  network_anonymization_key_ = value;
}

blink::mojom::ResourceType BraveRequestInfo::resource_type() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return resource_type_;
}

void BraveRequestInfo::set_resource_type(blink::mojom::ResourceType value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  resource_type_ = value;
}

const std::string& BraveRequestInfo::upload_data() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return upload_data_;
}

void BraveRequestInfo::set_upload_data(const std::string& value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  upload_data_ = value;
}

const std::optional<std::string>& BraveRequestInfo::devtools_request_id()
    const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return devtools_request_id_;
}

void BraveRequestInfo::set_devtools_request_id(
    const std::optional<std::string>& value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  devtools_request_id_ = value;
}

GURL* BraveRequestInfo::new_url() const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return new_url_;
}

void BraveRequestInfo::set_new_url(GURL* value) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  new_url_ = value;
}

// static
std::unique_ptr<brave::BraveRequestInfo> BraveRequestInfo::MakeCTX(
    const network::ResourceRequest& request,
    content::GlobalRenderFrameHostToken render_frame_token,
    uint64_t request_identifier,
    content::BrowserContext* browser_context,
    brave::BraveRequestInfo* old_ctx) {
  auto ctx = std::make_unique<brave::BraveRequestInfo>();
  ctx->set_request_identifier(request_identifier);
  ctx->set_method(request.method);
  ctx->set_request_url(request.url);
  // TODO(iefremov): Replace GURL with Origin
  ctx->set_initiator_url(
      request.request_initiator.value_or(url::Origin()).GetURL());

  ctx->set_referrer(request.referrer);
  ctx->set_referrer_policy(request.referrer_policy);

  ctx->set_resource_type(
      static_cast<blink::mojom::ResourceType>(request.resource_type));

  ctx->set_render_frame_token(render_frame_token);

  // TODO(iefremov): remove tab_url. Change tab_origin from GURL to Origin.
  // ctx->set_tab_url(request.top_frame_origin);
  if (request.trusted_params) {
    // TODO(iefremov): Turns out it provides us a not expected value for
    // cross-site top-level navigations. Fortunately for now it is not a problem
    // for shields functionality. We should reconsider this machinery, also
    // given that this is always empty for subresources.
    ctx->set_network_anonymization_key(
        request.trusted_params->isolation_info.network_anonymization_key());
    ctx->set_tab_origin(
        request.trusted_params->isolation_info.top_frame_origin()
            .value_or(url::Origin())
            .GetURL());
  }
  // TODO(iefremov): We still need this for WebSockets, currently
  // |AddChannelRequest| provides only old-fashioned |site_for_cookies|.
  // (See |BraveProxyingWebSocket|).
  if (ctx->tab_origin().is_empty()) {
    content::WebContents* contents = content::WebContents::FromRenderFrameHost(
        content::RenderFrameHost::FromFrameToken(ctx->render_frame_token()));
    if (contents) {
      ctx->set_tab_origin(
          url::Origin::Create(contents->GetLastCommittedURL()).GetURL());
    }
  }

  if (old_ctx) {
    ctx->set_internal_redirect(old_ctx->internal_redirect());
    ctx->set_redirect_source(old_ctx->redirect_source());
  }

  Profile* profile = Profile::FromBrowserContext(browser_context);
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile);
  ctx->set_allow_brave_shields(
      map ? brave_shields::GetBraveShieldsEnabled(map, ctx->tab_origin())
          : true);
  ctx->set_allow_ads(map &&
                     brave_shields::GetAdControlType(map, ctx->tab_origin()) ==
                         brave_shields::ControlType::ALLOW);
  // Currently, "aggressive" mode is registered as a cosmetic filtering control
  // type, even though it can also affect network blocking.
  ctx->set_aggressive_blocking(
      map && brave_shields::GetCosmeticFilteringControlType(
                 map, ctx->tab_origin()) == brave_shields::ControlType::BLOCK);

  // HACK: after we fix multiple creations of BraveRequestInfo we should
  // use only tab_origin. Since we recreate BraveRequestInfo during consequent
  // stages of navigation, |tab_origin| changes and so does |allow_referrers|
  // flag, which is not what we want for determining referrers.
  ctx->set_allow_referrers(map && brave_shields::AreReferrersAllowed(
                                      map, ctx->redirect_source().is_empty()
                                               ? ctx->tab_origin()
                                               : ctx->redirect_source()));
  ctx->set_upload_data(GetUploadData(request));

  ctx->set_browser_context(browser_context);

  // TODO(fmarier): remove this once the hacky code in
  // brave_proxying_url_loader_factory.cc is refactored. See
  // BraveProxyingURLLoaderFactory::InProgressRequest::UpdateRequestInfo().
  if (old_ctx) {
    ctx->set_internal_redirect(old_ctx->internal_redirect());
    ctx->set_redirect_source(old_ctx->redirect_source());
  }

  ctx->set_devtools_request_id(request.devtools_request_id);

  return ctx;
}

}  // namespace brave
