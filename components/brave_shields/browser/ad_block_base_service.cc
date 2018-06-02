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

  FilterOption current_option = FONoFilterOption;
  content::ResourceType internalResource = (content::ResourceType)resource_type;
  if (content::RESOURCE_TYPE_STYLESHEET == internalResource) {
    current_option = FOStylesheet;
  } else if (content::RESOURCE_TYPE_IMAGE == internalResource) {
    current_option = FOImage;
  } else if (content::RESOURCE_TYPE_SCRIPT == internalResource) {
    current_option = FOScript;
  }

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
