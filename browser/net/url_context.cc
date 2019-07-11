/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */


#include "brave/browser/net/url_context.h"

#include <memory>
#include <string>

#include "brave/common/extensions/extension_constants.h"
#include "brave/common/pref_names.h"
#include "brave/common/url_constants.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_shields/browser/brave_shields_web_contents_observer.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_io_data.h"
#include "components/prefs/testing_pref_service.h"
#include "content/public/browser/resource_request_info.h"
#include "extensions/browser/info_map.h"
#include "net/base/upload_bytes_element_reader.h"
#include "net/base/upload_data_stream.h"

namespace brave {

namespace {
bool IsWebTorrentDisabled(content::ResourceContext* resource_context) {
  DCHECK(resource_context);

  const ProfileIOData* io_data =
    ProfileIOData::FromResourceContext(resource_context);
  if (!io_data) {
    return false;
  }

  const extensions::InfoMap* infoMap = io_data->GetExtensionInfoMap();
  if (!infoMap) {
    return false;
  }

  return !infoMap->extensions().Contains(brave_webtorrent_extension_id) ||
    infoMap->disabled_extensions().Contains(brave_webtorrent_extension_id);
}

std::string GetUploadDataFromURLRequest(const net::URLRequest* request) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (!request->has_upload())
    return {};

  const net::UploadDataStream* stream = request->get_upload();
  if (!stream->GetElementReaders())
    return {};

  const auto* element_readers = stream->GetElementReaders();
  if (element_readers->empty())
    return {};

  std::string upload_data;
  for (const auto& element_reader : *element_readers) {
    const net::UploadBytesElementReader* reader =
        element_reader->AsBytesReader();
    if (!reader) {
      return {};
    }
    upload_data.append(reader->bytes(), reader->length());
  }
  return upload_data;
}

std::string GetUploadData(const network::ResourceRequest& request) {
  std::string upload_data;
  if (!request.request_body) {
    return {};
  }
  const auto* elements = request.request_body->elements();
  for (const network::DataElement& element : *elements) {
    if (element.type() == network::mojom::DataElementType::kBytes) {
      upload_data.append(element.bytes(), element.length());
    }
  }

  return upload_data;
}

}  // namespace

BraveRequestInfo::BraveRequestInfo() = default;

BraveRequestInfo::~BraveRequestInfo() = default;

void BraveRequestInfo::FillCTXFromRequest(const net::URLRequest* request,
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  ctx->request_identifier = request->identifier();
  ctx->request_url = request->url();
  if (request->initiator().has_value()) {
    ctx->initiator_url = request->initiator()->GetURL();
  }

  ctx->referrer = GURL(request->referrer());
  ctx->referrer_policy = request->referrer_policy();

  auto* request_info = content::ResourceRequestInfo::ForRequest(request);
  if (request_info) {
    ctx->resource_type = request_info->GetResourceType();
    if (auto* context = request_info->GetContext()) {
      ctx->is_webtorrent_disabled = IsWebTorrentDisabled(context);
    }
  }

  brave_shields::GetRenderFrameInfo(request,
                                    &ctx->render_frame_id,
                                    &ctx->render_process_id,
                                    &ctx->frame_tree_node_id);
  if (!request->site_for_cookies().is_empty()) {
    ctx->tab_url = request->site_for_cookies();
  } else {
    // We can not always use site_for_cookies since it can be empty in certain
    // cases. See the comments in url_request.h
    ctx->tab_url = brave_shields::BraveShieldsWebContentsObserver::
        GetTabURLFromRenderFrameInfo(ctx->render_process_id,
                                     ctx->render_frame_id,
                                     ctx->frame_tree_node_id).GetOrigin();
  }
  ctx->tab_origin = ctx->tab_url.GetOrigin();
  ctx->allow_brave_shields = brave_shields::IsAllowContentSettingFromIO(
      request, ctx->tab_origin, ctx->tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kBraveShields) &&
    !request->site_for_cookies().SchemeIs(kChromeExtensionScheme);
  ctx->allow_ads = brave_shields::IsAllowContentSettingFromIO(
      request, ctx->tab_origin, ctx->tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kAds);
  ctx->allow_http_upgradable_resource =
      brave_shields::IsAllowContentSettingFromIO(request, ctx->tab_origin,
          ctx->tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kHTTPUpgradableResources);
  ctx->allow_1p_cookies = brave_shields::IsAllowContentSettingFromIO(
      request, ctx->tab_origin, GURL("https://firstParty/"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);
  ctx->allow_3p_cookies = brave_shields::IsAllowContentSettingFromIO(
      request, ctx->tab_origin, GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kCookies);
  ctx->allow_referrers = brave_shields::IsAllowContentSettingFromIO(
      request, ctx->tab_origin, ctx->tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kReferrers);

  ctx->upload_data = GetUploadDataFromURLRequest(request);
}

// static
void BraveRequestInfo::FillCTX(
    const network::ResourceRequest& request,
    int render_process_id,
    int frame_tree_node_id,
    uint64_t request_identifier,
    content::ResourceContext* resource_context,
    std::shared_ptr<brave::BraveRequestInfo> ctx) {
  ctx->request_identifier = request_identifier;
  ctx->request_url = request.url;
  // TODO(iefremov): Replace GURL with Origin
  ctx->initiator_url =
      request.request_initiator.value_or(url::Origin()).GetURL();

  ctx->referrer = request.referrer;
  ctx->referrer_policy = request.referrer_policy;

  ctx->resource_type =
      static_cast<content::ResourceType>(request.resource_type);

  ctx->is_webtorrent_disabled = IsWebTorrentDisabled(resource_context);

  ctx->render_frame_id = request.render_frame_id;
  ctx->render_process_id = render_process_id;
  ctx->frame_tree_node_id = frame_tree_node_id;

  // TODO(iefremov): remove tab_url. Change tab_origin from GURL to Origin.
  // ctx->tab_url = request.top_frame_origin;
  ctx->tab_origin = request.top_frame_origin.value_or(url::Origin()).GetURL();

  ProfileIOData* io_data =
      ProfileIOData::FromResourceContext(resource_context);

  ctx->allow_brave_shields = brave_shields::IsAllowContentSettingWithIOData(
      io_data, ctx->tab_origin, ctx->tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kBraveShields) &&
    !ctx->tab_origin.SchemeIs(kChromeExtensionScheme);
  ctx->allow_ads = brave_shields::IsAllowContentSettingWithIOData(
      io_data, ctx->tab_origin, ctx->tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kAds);
  ctx->allow_http_upgradable_resource =
      brave_shields::IsAllowContentSettingWithIOData(io_data, ctx->tab_origin,
          ctx->tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kHTTPUpgradableResources);
  ctx->allow_1p_cookies = brave_shields::IsAllowContentSettingWithIOData(
      io_data, ctx->tab_origin, GURL("https://firstParty/"),
      CONTENT_SETTINGS_TYPE_PLUGINS, brave_shields::kCookies);
  ctx->allow_3p_cookies = brave_shields::IsAllowContentSettingWithIOData(
      io_data, ctx->tab_origin, GURL(), CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kCookies);
  ctx->allow_referrers = brave_shields::IsAllowContentSettingWithIOData(
      io_data, ctx->tab_origin, ctx->tab_origin, CONTENT_SETTINGS_TYPE_PLUGINS,
      brave_shields::kReferrers);

  ctx->upload_data = GetUploadData(request);
}


}  // namespace brave
