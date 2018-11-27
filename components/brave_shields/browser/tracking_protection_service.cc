/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/tracking_protection_service.h"

#include <algorithm>
#include <utility>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/thread_restrictions.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/dat_file_util.h"
#include "brave/vendor/tracking-protection/TPParser.h"

#define DAT_FILE "TrackingProtection.dat"
#define DAT_FILE_VERSION "1"
#define THIRD_PARTY_HOSTS_CACHE_SIZE 20

namespace brave_shields {

std::string TrackingProtectionService::g_tracking_protection_component_id_(
    kTrackingProtectionComponentId);
std::string TrackingProtectionService::g_tracking_protection_component_base64_public_key_(
    kTrackingProtectionComponentBase64PublicKey);

TrackingProtectionService::TrackingProtectionService()
  : tracking_protection_client_(new CTPParser()),
    // See comment in tracking_protection_service.h for white_list_
    white_list_({
      "connect.facebook.net",
      "connect.facebook.com",
      "staticxx.facebook.com",
      "www.facebook.com",
      "scontent.xx.fbcdn.net",
      "pbs.twimg.com",
      "scontent-sjc2-1.xx.fbcdn.net",
      "platform.twitter.com",
      "syndication.twitter.com",
      "cdn.syndication.twimg.com"
    }),
    weak_factory_(this) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

TrackingProtectionService::~TrackingProtectionService() {
  Cleanup();
}

void TrackingProtectionService::Cleanup() {
  tracking_protection_client_.reset();
}

bool TrackingProtectionService::ShouldStartRequest(const GURL& url,
    content::ResourceType resource_type,
    const std::string &tab_host) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::string host = url.host();
  if (!tracking_protection_client_->matchesTracker(
        tab_host.c_str(), host.c_str())) {
    return true;
  }

  std::vector<std::string> hosts(GetThirdPartyHosts(tab_host));
  for (size_t i = 0; i < hosts.size(); i++) {
    if (host == hosts[i] ||
        host.find((std::string)"." + hosts[i]) != std::string::npos) {
      return true;
    }
    size_t iPos = host.find((std::string)"." + hosts[i]);
    if (iPos == std::string::npos) {
      continue;
    }
    if (hosts[i].length() + ((std::string)".").length() + iPos ==
        host.length()) {
      return true;
    }
  }

  if (std::find(white_list_.begin(), white_list_.end(), host) !=
      white_list_.end()) {
    return true;
  }
  return false;
}

bool TrackingProtectionService::Init() {
  Register(kTrackingProtectionComponentName,
           g_tracking_protection_component_id_,
           g_tracking_protection_component_base64_public_key_);
  return true;
}

void TrackingProtectionService::OnDATFileDataReady() {
  if (buffer_.empty()) {
    LOG(ERROR) << "Could not obtain tracking protection data";
    return;
  }
  tracking_protection_client_.reset(new CTPParser());
  if (!tracking_protection_client_->deserialize((char*)&buffer_.front())) {
    tracking_protection_client_.reset();
    LOG(ERROR) << "Failed to deserialize tracking protection data";
  }
}

void TrackingProtectionService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  base::FilePath dat_file_path =
      install_dir.AppendASCII(DAT_FILE_VERSION).AppendASCII(DAT_FILE);

  GetTaskRunner()->PostTaskAndReply(
      FROM_HERE,
      base::Bind(&GetDATFileData, dat_file_path, &buffer_),
      base::Bind(&TrackingProtectionService::OnDATFileDataReady,
                 weak_factory_.GetWeakPtr()));
}

// Ported from Android: net/blockers/blockers_worker.cc
std::vector<std::string>
TrackingProtectionService::GetThirdPartyHosts(const std::string& base_host) {
  {
    std::lock_guard<std::mutex> guard(third_party_hosts_mutex_);
    std::map<std::string, std::vector<std::string>>::const_iterator iter =
      third_party_hosts_cache_.find(base_host);
    if (third_party_hosts_cache_.end() != iter) {
      if (third_party_base_hosts_.size() != 0
          && third_party_base_hosts_[third_party_hosts_cache_.size() - 1] !=
          base_host) {
        for (size_t i = 0; i < third_party_base_hosts_.size(); i++) {
          if (third_party_base_hosts_[i] == base_host) {
            third_party_base_hosts_.erase(third_party_base_hosts_.begin() + i);
            third_party_base_hosts_.push_back(base_host);
            break;
          }
        }
      }
      return iter->second;
    }
  }

  char* thirdPartyHosts =
    tracking_protection_client_->findFirstPartyHosts(base_host.c_str());
  std::vector<std::string> hosts;
  if (nullptr != thirdPartyHosts) {
    std::string strThirdPartyHosts = thirdPartyHosts;
    size_t iPos = strThirdPartyHosts.find(",");
    while (iPos != std::string::npos) {
      std::string thirdParty = strThirdPartyHosts.substr(0, iPos);
      strThirdPartyHosts = strThirdPartyHosts.substr(iPos + 1);
      iPos = strThirdPartyHosts.find(",");
      hosts.push_back(thirdParty);
    }
    if (0 != strThirdPartyHosts.length()) {
      hosts.push_back(strThirdPartyHosts);
    }
    delete []thirdPartyHosts;
  }

  {
    std::lock_guard<std::mutex> guard(third_party_hosts_mutex_);
    if (third_party_hosts_cache_.size() == THIRD_PARTY_HOSTS_CACHE_SIZE &&
        third_party_base_hosts_.size() == THIRD_PARTY_HOSTS_CACHE_SIZE) {
      third_party_hosts_cache_.erase(third_party_base_hosts_[0]);
      third_party_base_hosts_.erase(third_party_base_hosts_.begin());
    }
    third_party_base_hosts_.push_back(base_host);
    third_party_hosts_cache_.insert(
        std::pair<std::string, std::vector<std::string>>(base_host, hosts));
  }

  return hosts;
}

// static
void TrackingProtectionService::SetComponentIdAndBase64PublicKeyForTest(
    const std::string& component_id,
    const std::string& component_base64_public_key) {
  g_tracking_protection_component_id_ = component_id;
  g_tracking_protection_component_base64_public_key_ = component_base64_public_key;
}

scoped_refptr<base::SequencedTaskRunner> TrackingProtectionService::GetTaskRunner() {
  // We share the same task runner for all ad-block and TP code
  return g_brave_browser_process->ad_block_service()->GetTaskRunner();
}

///////////////////////////////////////////////////////////////////////////////

// The brave shields factory. Using the Brave Shields as a singleton
// is the job of the browser process.
std::unique_ptr<TrackingProtectionService> TrackingProtectionServiceFactory() {
  return std::make_unique<TrackingProtectionService>();
}

}  // namespace brave_shields
