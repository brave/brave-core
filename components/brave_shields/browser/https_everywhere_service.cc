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
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/utf_string_conversions.h"
#include "base/threading/scoped_blocking_call.h"
#include "base/values.h"
#include "third_party/leveldatabase/src/include/leveldb/db.h"
#include "third_party/re2/src/re2/re2.h"
#include "third_party/zlib/google/zip.h"

#define DAT_FILE "httpse.leveldb.zip"
#define DAT_FILE_VERSION "6.0"
#define HTTPSE_URLS_REDIRECTS_COUNT_QUEUE   1
#define HTTPSE_URL_MAX_REDIRECTS_COUNT      5

namespace {

std::vector<std::string> Split(const std::string& s, char delim) {
  std::stringstream ss(s);
  std::string item;
  std::vector<std::string> result;
  while (getline(ss, item, delim)) {
    result.push_back(item);
  }
  return result;
}

// returns parts in reverse order, makes list of lookup domains like com.foo.*
std::vector<std::string> ExpandDomainForLookup(const std::string& domain) {
  std::vector<std::string> resultDomains;
  std::vector<std::string> domainParts = Split(domain, '.');
  if (domainParts.empty()) {
    return resultDomains;
  }

  for (size_t i = 0; i < domainParts.size() - 1; i++) {
    // i < size()-1 is correct: don't want 'com.*' added to resultDomains
    std::string slice = "";
    std::string dot = "";
    for (int j = domainParts.size() - 1; j >= static_cast<int>(i); j--) {
      slice += dot + domainParts[j];
      dot = ".";
    }
    if (0 != i) {
      // We don't want * on the top URL
      resultDomains.push_back(slice + ".*");
    } else {
      resultDomains.push_back(slice);
    }
  }
  return resultDomains;
}
std::string leveldbGet(leveldb::DB* db, const std::string &key) {
  if (!db) {
    return "";
  }

  std::string value;
  leveldb::Status s = db->Get(leveldb::ReadOptions(), key, &value);
  return s.ok() ? value : "";
}

}  // namespace

namespace brave_shields {

HTTPSEverywhereService::Engine::Engine(HTTPSEverywhereService* service)
    : level_db_(nullptr), service_(service) {
  DETACH_FROM_SEQUENCE(sequence_checker_);
}

void HTTPSEverywhereService::Engine::Init(const base::FilePath& base_dir) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  base::FilePath zip_db_file_path =
      base_dir.AppendASCII(DAT_FILE_VERSION).AppendASCII(DAT_FILE);
  base::FilePath unzipped_level_db_path = zip_db_file_path.RemoveExtension();
  base::FilePath destination = zip_db_file_path.DirName();
  if (!zip::Unzip(zip_db_file_path, destination)) {
    LOG(ERROR) << "Failed to unzip database file "
               << zip_db_file_path.value().c_str();
    return;
  }

  CloseDatabase();

  leveldb::Options options;
  leveldb::Status status =
      leveldb::DB::Open(options,
                        unzipped_level_db_path.AsUTF8Unsafe(),
                        &level_db_);
  if (!status.ok() || !level_db_) {
    LOG(ERROR) << "Level db open error "
               << unzipped_level_db_path.value().c_str()
               << ", error: " << status.ToString();
    CloseDatabase();
    return;
  }
}

bool HTTPSEverywhereService::Engine::GetHTTPSURL(
    const GURL* url,
    const uint64_t& request_identifier,
    std::string* new_url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!url->is_valid())
    return false;

  if (!level_db_ || url->scheme() == url::kHttpsScheme) {
    return false;
  }

  if (!service_->ShouldHTTPSERedirect(request_identifier)) {
    return false;
  }

  if (service_->recently_used_cache().get(url->spec(), new_url)) {
    service_->AddHTTPSEUrlToRedirectList(request_identifier);
    return true;
  }

  GURL candidate_url(*url);
  if (g_ignore_port_for_test_ && candidate_url.has_port()) {
    GURL::Replacements replacements;
    replacements.ClearPort();
    candidate_url = candidate_url.ReplaceComponents(replacements);
  }

  SCOPED_UMA_HISTOGRAM_TIMER("Brave.HTTPSE.GetHTTPSURL");
  const std::vector<std::string> domains =
      ExpandDomainForLookup(candidate_url.host());
  for (auto domain : domains) {
    std::string value = leveldbGet(level_db_, domain);
    if (!value.empty()) {
      *new_url = ApplyHTTPSRule(candidate_url.spec(), value);
      if (0 != new_url->length()) {
        service_->recently_used_cache().add(candidate_url.spec(), *new_url);
        service_->AddHTTPSEUrlToRedirectList(request_identifier);
        return true;
      }
    }
  }
  service_->recently_used_cache().remove(candidate_url.spec());
  return false;
}

std::string HTTPSEverywhereService::Engine::ApplyHTTPSRule(
    const std::string& originalUrl,
    const std::string& rule) {
  absl::optional<base::Value> json_object = base::JSONReader::Read(rule);
  if (absl::nullopt == json_object || !json_object->is_list()) {
    return "";
  }

  base::Value::ConstListView topValues = json_object->GetListDeprecated();
  for (auto it = topValues.cbegin(); it != topValues.cend(); ++it) {
    if (!it->is_dict()) {
      continue;
    }
    const base::DictionaryValue* childTopDictionary = nullptr;
    it->GetAsDictionary(&childTopDictionary);
    if (nullptr == childTopDictionary) {
      continue;
    }

    const base::Value* exclusion = nullptr;
    if (childTopDictionary->Get("e", &exclusion)) {
      const base::ListValue* eValues = nullptr;
      exclusion->GetAsList(&eValues);
      if (nullptr != eValues) {
        for (size_t j = 0; j < eValues->GetListDeprecated().size(); ++j) {
          const auto& list = eValues->GetListDeprecated();
          if (list.size() <= j) {
            continue;
          }
          const base::DictionaryValue* pDictionary = nullptr;
          list[j].GetAsDictionary(&pDictionary);
          if (nullptr == pDictionary) {
            continue;
          }
          const base::Value* patternValue = nullptr;
          if (!pDictionary->Get("p", &patternValue)) {
            continue;
          }
          if (!patternValue->is_string()) {
            continue;
          }
          std::string pattern =
              CorrecttoRuleToRE2Engine(patternValue->GetString());
          if (RE2::FullMatch(originalUrl, pattern)) {
            return "";
          }
        }
      }
    }

    const base::Value* rules = nullptr;
    if (!childTopDictionary->Get("r", &rules)) {
      return "";
    }
    const base::ListValue* rValues = nullptr;
    rules->GetAsList(&rValues);
    if (nullptr == rValues) {
      return "";
    }

    for (size_t j = 0; j < rValues->GetListDeprecated().size(); ++j) {
      const auto& list = rValues->GetListDeprecated();
      if (list.size() <= j) {
        continue;
      }
      const base::DictionaryValue* pDictionary = nullptr;
      list[j].GetAsDictionary(&pDictionary);
      if (nullptr == pDictionary) {
        continue;
      }
      const base::Value* patternValue = nullptr;
      if (pDictionary->Get("d", &patternValue)) {
        std::string newUrl(originalUrl);
        return newUrl.insert(4, "s");
      }

      const base::Value* from_value = nullptr;
      const base::Value* to_value = nullptr;
      if (!pDictionary->Get("f", &from_value) ||
          !pDictionary->Get("t", &to_value)) {
        continue;
      }
      if (!from_value->is_string() || !to_value->is_string()) {
        continue;
      }

      std::string to = CorrecttoRuleToRE2Engine(to_value->GetString());
      std::string newUrl(originalUrl);
      RE2 regExp(from_value->GetString());

      if (RE2::Replace(&newUrl, regExp, to) && newUrl != originalUrl) {
        return newUrl;
      }
    }
  }
  return "";
}

std::string HTTPSEverywhereService::Engine::CorrecttoRuleToRE2Engine(
    const std::string& to) {
  std::string correctedto(to);
  size_t pos = to.find("$");
  while (std::string::npos != pos) {
    correctedto[pos] = '\\';
    pos = correctedto.find("$");
  }

  return correctedto;
}

void HTTPSEverywhereService::Engine::CloseDatabase() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (level_db_) {
    delete level_db_;
    level_db_ = nullptr;
  }
}

bool HTTPSEverywhereService::g_ignore_port_for_test_(false);

HTTPSEverywhereService::HTTPSEverywhereService(
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : BaseBraveShieldsService(task_runner),
      engine_(new Engine(this), base::OnTaskRunnerDeleter(task_runner)) {}

HTTPSEverywhereService::~HTTPSEverywhereService() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

bool HTTPSEverywhereService::Init() {
  return true;
}

void HTTPSEverywhereService::InitDB(const base::FilePath& install_dir) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  GetTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(&Engine::Init, engine_->AsWeakPtr(), install_dir));
}

bool HTTPSEverywhereService::GetHTTPSURLFromCacheOnly(
    const GURL* url,
    const uint64_t& request_identifier,
    std::string* cached_url) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
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

HTTPSERecentlyUsedCache<std::string>&
HTTPSEverywhereService::recently_used_cache() {
  return recently_used_cache_;
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
      httpse_urls_redirects_count_.erase(httpse_urls_redirects_count_.begin());
    }
    httpse_urls_redirects_count_.push_back(
        HTTPSE_REDIRECTS_COUNT_ST(request_identifier, 1));
  }
}

// static
void HTTPSEverywhereService::SetIgnorePortForTest(bool ignore) {
  g_ignore_port_for_test_ = ignore;
}

///////////////////////////////////////////////////////////////////////////////

// The brave shields factory. Using the Brave Shields as a singleton
// is the job of the browser process.
std::unique_ptr<HTTPSEverywhereService> HTTPSEverywhereServiceFactory(
    scoped_refptr<base::SequencedTaskRunner> task_runner) {
  return std::make_unique<HTTPSEverywhereService>(task_runner);
}

}  // namespace brave_shields
