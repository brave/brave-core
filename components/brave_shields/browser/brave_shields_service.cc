/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/brave_shields_service.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "brave/vendor/ad-block/ad_block_client.h"
#include "chrome/common/chrome_paths.h"
#include "url/gurl.h"

#define ADBLOCK_DATA_FILE "ABPFilterParserData.dat"

namespace brave_shields {

BraveShieldsService::BraveShieldsService() :
    initialized_(false),
    ad_block_client_(new AdBlockClient()) {
  Start();
}

BraveShieldsService::~BraveShieldsService() {
}

bool BraveShieldsService::Start() {
  if (initialized_) {
    return true;
  }
  return InitAdBlock();
}

void BraveShieldsService::Stop() {
}

bool BraveShieldsService::Check(const std::string &spec,
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

  if (ad_block_client_->matches(spec.c_str(),
        currentOption,
        initiator_host.c_str())) {
    // LOG(ERROR) << "BraveShieldsService::Check(), host: " << initiator_host
    //  << ", resource type: " << resource_type
    //  << ", spec: " << spec;
    return true;
  }

  return false;
}

bool BraveShieldsService::InitAdBlock() {
  base::ThreadRestrictions::AssertIOAllowed();
  std::lock_guard<std::mutex> guard(adblock_init_mutex_);
  if (!GetData(ADBLOCK_DATA_FILE, adblock_buffer_)) {
    LOG(ERROR) << "Could not obtain ad block data file";
    return false;
  }

  if (!ad_block_client_->deserialize((char*)&adblock_buffer_.front())) {
    ad_block_client_.reset();
    LOG(ERROR) << "BraveShieldsService::InitAdBlock deserialize failed";
    return false;
  }

  set_adblock_initialized();
  return true;
}

void BraveShieldsService::set_adblock_initialized() {
  std::lock_guard<std::mutex> guard(initialized_mutex_);
  initialized_ = true;
}

bool BraveShieldsService::GetData(
      const std::string& fileName,
      std::vector<unsigned char>& buffer) {
    base::FilePath app_data_path;
    PathService::Get(chrome::DIR_USER_DATA, &app_data_path);

  base::FilePath dataFilePath = app_data_path.Append(fileName);
  int64_t size = 0;
  if (!base::PathExists(dataFilePath)
      || !base::GetFileSize(dataFilePath, &size)
      || 0 == size) {
    LOG(ERROR) << "BraveShieldsService::GetData:"
      << "the dat file is not found or corrupted "
      << dataFilePath;
    return false;
  }
  buffer.resize(size);
  if (size != base::ReadFile(dataFilePath, (char*)&buffer.front(), size)) {
    LOG(ERROR) << "BraveShieldsService::GetData: cannot "
      << "read dat file " << fileName;
     return false;
  }

  LOG(ERROR) << "Initialized brave shields service correctly";
  return true;
}


///////////////////////////////////////////////////////////////////////////////

// The brave sheilds factory. Using the Brave Sheilds as a singleton
// is the job of the browser process.
// TODO(bbondy): consider making this a singleton.
std::unique_ptr<BraveShieldsService> BraveShieldsServiceFactory() {
  return base::MakeUnique<BraveShieldsService>();
}

}  // namespace brave_shields
