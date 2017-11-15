/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/ad_block_service.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "brave/components/brave_shields/browser/dat_file_util.h"
#include "brave/vendor/ad-block/ad_block_client.h"

#define DATA_FILE "ABPFilterParserData.dat"

namespace brave_shields {

AdBlockService::AdBlockService() :
    ad_block_client_(new AdBlockClient()) {
}

AdBlockService::~AdBlockService() {
  Cleanup();
}

void AdBlockService::Cleanup() {
  ad_block_client_.reset();
}

bool AdBlockService::Check(const GURL& url,
    content::ResourceType resource_type,
    const std::string &initiator_host) {

  FilterOption currentOption = FONoFilterOption;
  content::ResourceType internalResource = (content::ResourceType)resource_type;
  if (content::RESOURCE_TYPE_STYLESHEET == internalResource) {
    currentOption = FOStylesheet;
  } else if (content::RESOURCE_TYPE_IMAGE == internalResource) {
    currentOption = FOImage;
  } else if (content::RESOURCE_TYPE_SCRIPT == internalResource) {
    currentOption = FOScript;
  }

  if (ad_block_client_->matches(url.spec().c_str(),
        currentOption,
        initiator_host.c_str())) {
    // LOG(ERROR) << "AdBlockService::Check(), host: " << initiator_host
    //  << ", resource type: " << resource_type
    //  << ", url.spec(): " << url.spec();
    return true;
  }

  return false;
}

bool AdBlockService::Init() {
   if (!GetDATFileData(DATA_FILE, buffer_)) {
    LOG(ERROR) << "Could not obtain ad block data file";
    return false;
  }
  if (!ad_block_client_->deserialize((char*)&buffer_.front())) {
    ad_block_client_.reset();
    LOG(ERROR) << "AdBlockService::InitAdBlock deserialize failed";
    return false;
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////

// The brave sheilds factory. Using the Brave Sheilds as a singleton
// is the job of the browser process.
// TODO(bbondy): consider making this a singleton.
std::unique_ptr<BaseBraveShieldsService> AdBlockServiceFactory() {
  return base::MakeUnique<AdBlockService>();
}

}  // namespace brave_shields
