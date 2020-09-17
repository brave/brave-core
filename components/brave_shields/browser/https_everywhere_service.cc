/* Copyright 2016 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/https_everywhere_service.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "base/base_paths.h"
#include "base/bind.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/threading/scoped_blocking_call.h"
#include "brave/browser/component_updater/brave_component_installer.h"
#include "brave/common/pref_names.h"
#include "brave/vendor/https-everywhere-lib-cpp/src/wrapper.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/first_run/first_run.h"
#include "components/component_updater/component_updater_service.h"
#include "components/prefs/pref_service.h"
#include "third_party/zlib/google/zip.h"

#define DAT_FILE "httpse-rs.json.zip"
#define DAT_FILE_VERSION "7.0"
#define HTTPSE_URLS_REDIRECTS_COUNT_QUEUE   1
#define HTTPSE_URL_MAX_REDIRECTS_COUNT      5

namespace {

// Legacy component ID, unregistered at startup
const char kHTTPSEverywhere6ComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAvn9zSMjTmhkQyrZu5UdN"
    "350nPqLoSeCYngcC7yDFwaUHjoBQXCZqGeDC69ciCQ2mlRhcV2nxXqlUDkiC6+7m"
    "651nI+gi4oVqHagc7EFUyGA0yuIk7qIMvCBdH7wbET27de0rzbRzRht9EKzEjIhC"
    "BtoPnmyrO/8qPrH4XR4cPfnFPuJssBBxC1B35H7rh0Br9qePhPDDe9OjyqYxPuio"
    "+YcC9obL4g5krVrfrlKLfFNpIewUcJyBpSlCgfxEyEhgDkK9cILTMUi5vC7GxS3P"
    "OtZqgfRg8Da4i+NwmjQqrz0JFtPMMSyUnmeMj+mSOL4xZVWr8fU2/GOCXs9gczDp"
    "JwIDAQAB";

void NoopReadyCallback(const base::FilePath&, const std::string&) {}

void OnLegacyHTTPSEverywhereRegistered(
    scoped_refptr<component_updater::ComponentInstaller> installer
) {
  installer->Uninstall();
}

void MigrateHTTPSEverywhere6Prefs() {
  auto* updater = g_browser_process->component_updater();
  auto installer = base::MakeRefCounted<component_updater::ComponentInstaller>(
      std::make_unique<brave::BraveComponentInstallerPolicy>(
          brave_shields::kHTTPSEverywhereComponentName,
          kHTTPSEverywhere6ComponentBase64PublicKey,
          base::BindRepeating(NoopReadyCallback)));
  installer->Register(updater,
      base::BindOnce(OnLegacyHTTPSEverywhereRegistered, installer));
}

}  // namespace

namespace brave_shields {

const char kHTTPSEverywhereComponentName[] = "Brave HTTPS Everywhere Updater";
const char kHTTPSEverywhereComponentId[] = "nceadfeaijjaobpigjldlbaogfokgajf";
const char kHTTPSEverywhereComponentBase64PublicKey[] =
    "MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAnlSgb99knPRN5YtflR0x"
    "gi6eO3K4XYzF1shgACd40ccwSGyuYwqdBK6f8jAJOL5leBnmvXgHTeiXc1K+l0K8"
    "FgQMO9Y1EtzBHfdXxN3u7NKoQmCO9m5dlz771m4Qgg+JtB6hsmqNAMkqee50wQn3"
    "q8+tcS8xM63zOGM0E9ub/tFAxPTAfLJlka2Qn0iFj7ON2OzfpJNQguPx4rJS34zi"
    "BfXagFF+tlNzdy0BOHIHOiqtZ09sOgHaptIZdWVef6Y9v4fjOlVNlrn45rt98whr"
    "YFAgbOutakZNf6PEQ/brga6zAhu6B/0kIOEFZhDC198gqqaAacV8bHmryUxjWPEi"
    "qwIDAQAB";

bool HTTPSEverywhereService::g_ignore_port_for_test_(false);
std::string HTTPSEverywhereService::g_https_everywhere_component_id_(
    kHTTPSEverywhereComponentId);
std::string
HTTPSEverywhereService::g_https_everywhere_component_base64_public_key_(
    kHTTPSEverywhereComponentBase64PublicKey);

HTTPSEverywhereService::HTTPSEverywhereService(
    BraveComponent::Delegate* delegate)
    : BaseBraveShieldsService(delegate),
      rust_client_(httpse::HttpsEverywhereClient()) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

HTTPSEverywhereService::~HTTPSEverywhereService() {
}

bool HTTPSEverywhereService::Init() {
  PrefService* local_state = g_browser_process->local_state();
  if (!local_state) {
    return false;
  }

  bool hasMigratedHTTPSEverywhere6 =
      local_state->GetBoolean(kMigratedHTTPSEverywhere6);

  if (!hasMigratedHTTPSEverywhere6) {
    local_state->SetBoolean(kMigratedHTTPSEverywhere6, true);
    if (!first_run::IsChromeFirstRun()) {
      MigrateHTTPSEverywhere6Prefs();
    }
  }

  Register(kHTTPSEverywhereComponentName,
           g_https_everywhere_component_id_,
           g_https_everywhere_component_base64_public_key_);
  return true;
}

void HTTPSEverywhereService::InitDB(const base::FilePath& install_dir) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  base::FilePath zip_file_path =
      install_dir.AppendASCII(DAT_FILE_VERSION).AppendASCII(DAT_FILE);
  base::FilePath json_ruleset_path = zip_file_path.RemoveExtension();
  base::FilePath destination = zip_file_path.DirName();
  if (!zip::Unzip(zip_file_path, destination)) {
    LOG(ERROR) << "Failed to unzip HTTPSE rules file "
               << zip_file_path.value().c_str();
    return;
  }

  std::string rules;
  base::ReadFileToString(json_ruleset_path, &rules);
  rust_client_.LoadRules(rules);
}

void HTTPSEverywhereService::OnComponentReady(
    const std::string& component_id,
    const base::FilePath& install_dir,
    const std::string& manifest) {
  GetTaskRunner()->PostTask(
      FROM_HERE,
      base::Bind(&HTTPSEverywhereService::InitDB,
                 AsWeakPtr(),
                 install_dir));
}

bool HTTPSEverywhereService::GetHTTPSURL(
    const GURL* url,
    const uint64_t& request_identifier,
    std::string* new_url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!url->is_valid())
    return false;

  if (!IsInitialized() || url->scheme() == url::kHttpsScheme) {
    return false;
  }
  if (!ShouldHTTPSERedirect(request_identifier)) {
    return false;
  }

  if (recently_used_cache_.get(url->spec(), new_url)) {
    AddHTTPSEUrlToRedirectList(request_identifier);
    return true;
  }

  GURL candidate_url(*url);
  if (g_ignore_port_for_test_ && candidate_url.has_port()) {
    GURL::Replacements replacements;
    replacements.ClearPort();
    candidate_url = candidate_url.ReplaceComponents(replacements);
  }

  auto rewrite_result = rust_client_.RewriteUrl(candidate_url.spec());
  if (rewrite_result.action == httpse::RewriteAction::REWRITE_URL) {
    *new_url = rewrite_result.new_url;
    recently_used_cache_.add(candidate_url.spec(), *new_url);
    AddHTTPSEUrlToRedirectList(request_identifier);
    return true;
  }
  recently_used_cache_.remove(candidate_url.spec());
  return false;
}

bool HTTPSEverywhereService::GetHTTPSURLFromCacheOnly(
    const GURL* url,
    const uint64_t& request_identifier,
    std::string* cached_url) {
  if (!url->is_valid())
    return false;

  if (!IsInitialized() || url->scheme() == url::kHttpsScheme) {
    return false;
  }
  if (!ShouldHTTPSERedirect(request_identifier)) {
    return false;
  }

  if (recently_used_cache_.get(url->spec(), cached_url)) {
    AddHTTPSEUrlToRedirectList(request_identifier);
    return true;
  }
  return false;
}

bool HTTPSEverywhereService::ShouldHTTPSERedirect(
    const uint64_t& request_identifier) {
  base::AutoLock auto_lock(httpse_get_urls_redirects_count_mutex_);
  for (size_t i = 0; i < httpse_urls_redirects_count_.size(); i++) {
    if (request_identifier ==
        httpse_urls_redirects_count_[i].request_identifier_ &&
        httpse_urls_redirects_count_[i].redirects_ >=
        HTTPSE_URL_MAX_REDIRECTS_COUNT - 1) {
      return false;
    }
  }

  return true;
}

void HTTPSEverywhereService::AddHTTPSEUrlToRedirectList(
    const uint64_t& request_identifier) {
  // Adding redirects count for the current request
  base::AutoLock auto_lock(httpse_get_urls_redirects_count_mutex_);
  bool hostFound = false;
  for (size_t i = 0; i < httpse_urls_redirects_count_.size(); i++) {
    if (request_identifier ==
        httpse_urls_redirects_count_[i].request_identifier_) {
      // Found the host, just increment the redirects_count
      httpse_urls_redirects_count_[i].redirects_++;
      hostFound = true;
      break;
    }
  }
  if (!hostFound) {
      // The host is new, adding it to the redirects list
      if (httpse_urls_redirects_count_.size() >=
          HTTPSE_URLS_REDIRECTS_COUNT_QUEUE) {
          // The queue is full, erase the first element
          httpse_urls_redirects_count_.erase(
              httpse_urls_redirects_count_.begin());
      }
      httpse_urls_redirects_count_.push_back(
          HTTPSE_REDIRECTS_COUNT_ST(request_identifier, 1));
  }
}

void HTTPSEverywhereService::CloseDatabase() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

// static
void HTTPSEverywhereService::SetComponentIdAndBase64PublicKeyForTest(
    const std::string& component_id,
    const std::string& component_base64_public_key) {
  g_https_everywhere_component_id_ = component_id;
  g_https_everywhere_component_base64_public_key_ = component_base64_public_key;
}

// static
void HTTPSEverywhereService::SetIgnorePortForTest(bool ignore) {
  g_ignore_port_for_test_ = ignore;
}

///////////////////////////////////////////////////////////////////////////////

// The brave shields factory. Using the Brave Shields as a singleton
// is the job of the browser process.
std::unique_ptr<HTTPSEverywhereService> HTTPSEverywhereServiceFactory(
    BraveComponent::Delegate* delegate) {
  return std::make_unique<HTTPSEverywhereService>(delegate);
}

void RegisterPrefsForHTTPSEverywhereService(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kMigratedHTTPSEverywhere6, false);
}

}  // namespace brave_shields
