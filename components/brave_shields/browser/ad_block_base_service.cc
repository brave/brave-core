/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_base_service.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/brave_shields/browser/dat_file_util.h"
#include "brave/vendor/ad-block/ad_block_client.h"


namespace {

FilterOption ResourceTypeToFilterOption(content::ResourceType resource_type) {
  FilterOption filter_option = FONoFilterOption;
  switch(resource_type) {
    // top level page
    case content::RESOURCE_TYPE_MAIN_FRAME:
      filter_option = FODocument;
      break;
    // frame or iframe
    case content::RESOURCE_TYPE_SUB_FRAME:
      filter_option = FOSubdocument;
      break;
    // a CSS stylesheet
    case content::RESOURCE_TYPE_STYLESHEET:
      filter_option = FOStylesheet;
      break;
    // an external script
    case content::RESOURCE_TYPE_SCRIPT:
      filter_option = FOScript;
      break;
    // an image (jpg/gif/png/etc)
    case content::RESOURCE_TYPE_IMAGE:
      filter_option = FOImage;
      break;
    // a font
    case content::RESOURCE_TYPE_FONT_RESOURCE:
      filter_option = FOFont;
      break;
    // an "other" subresource.
    case content::RESOURCE_TYPE_SUB_RESOURCE:
      filter_option = FOOther;
      break;
    // an object (or embed) tag for a plugin.
    case content::RESOURCE_TYPE_OBJECT:
      filter_option = FOObject;
      break;
    // a media resource.
    case content::RESOURCE_TYPE_MEDIA:
      filter_option = FOMedia;
      break;
    // a XMLHttpRequest
    case content::RESOURCE_TYPE_XHR:
      filter_option = FOXmlHttpRequest;
      break;
    // a ping request for <a ping>/sendBeacon.
    case content::RESOURCE_TYPE_PING:
      filter_option = FOPing;
      break;
    // the main resource of a dedicated
    case content::RESOURCE_TYPE_WORKER:
    // the main resource of a shared worker.
    case content::RESOURCE_TYPE_SHARED_WORKER:
    // an explicitly requested prefetch
    case content::RESOURCE_TYPE_PREFETCH:
    // a favicon
    case content::RESOURCE_TYPE_FAVICON:
    // the main resource of a service worker.
    case content::RESOURCE_TYPE_SERVICE_WORKER:
    // a report of Content Security Policy
    case content::RESOURCE_TYPE_CSP_REPORT:
    // a resource that a plugin requested.
    case content::RESOURCE_TYPE_PLUGIN_RESOURCE:
    case content::RESOURCE_TYPE_LAST_TYPE:
    default:
      break;
  }
  return filter_option;
}

}  // namespace

namespace brave_shields {

AdBlockBaseService::AdBlockBaseService()
    : BaseBraveShieldsService(),
      ad_block_client_(new AdBlockClient()),
      weak_factory_(this) {
}

AdBlockBaseService::~AdBlockBaseService() {
  Cleanup();
}

void AdBlockBaseService::Cleanup() {
  ad_block_client_.reset();
}

bool AdBlockBaseService::ShouldStartRequest(const GURL& url,
    content::ResourceType resource_type,
    const std::string& tab_host) {

  FilterOption current_option = ResourceTypeToFilterOption(resource_type);
  if (ad_block_client_->matches(url.spec().c_str(),
        current_option,
        tab_host.c_str())) {
    // LOG(ERROR) << "AdBlockBaseService::ShouldStartRequest(), host: " << tab_host
    //  << ", resource type: " << resource_type
    //  << ", url.spec(): " << url.spec();
    return false;
  }

  return true;
}

void AdBlockBaseService::GetDATFileData(const base::FilePath& dat_file_path) {
  GetTaskRunner()->PostTaskAndReply(
      FROM_HERE,
      base::Bind(&brave_shields::GetDATFileData, dat_file_path, &buffer_),
      base::Bind(&AdBlockBaseService::OnDATFileDataReady,
                 weak_factory_.GetWeakPtr()));
}

void AdBlockBaseService::OnDATFileDataReady() {
  if (buffer_.empty()) {
    LOG(ERROR) << "Could not obtain ad block data";
    return;
  }
  ad_block_client_.reset(new AdBlockClient());
  if (!ad_block_client_->deserialize((char*)&buffer_.front())) {
    ad_block_client_.reset();
    LOG(ERROR) << "Failed to deserialize ad block data";
    return;
  }
}

bool AdBlockBaseService::Init() {
  return true;
}

///////////////////////////////////////////////////////////////////////////////

}  // namespace brave_shields
