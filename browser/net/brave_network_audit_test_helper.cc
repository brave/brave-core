/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/net/brave_network_audit_test_helper.h"

#include "base/json/json_file_value_serializer.h"
#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave/browser/net/brave_network_audit_allowed_lists.h"
#include "chrome/test/base/in_process_browser_test.h"
#include "content/public/test/browser_test_utils.h"
#include "third_party/re2/src/re2/re2.h"

namespace brave {
namespace {
// Based on the implementation of isPrivateIP() from NPM's "ip" module.
// See https://github.com/indutny/node-ip/blob/master/lib/ip.js
constexpr const char* kPrivateIPRegexps[] = {
    "(::f{4}:)?10\\.([0-9]{1,3})\\.([0-9]{1,3})\\.([0-9]{1,3})",
    "(::f{4}:)?192\\.168\\.([0-9]{1,3})\\.([0-9]{1,3})",
    "(::f{4}:)?172\\.(1[6-9]|2\\d|30|31)\\.([0-9]{1,3})\\.([0-9]{1,3})",
    "(::f{4}:)?127\\.([0-9]{1,3})\\.([0-9]{1,3})\\.([0-9]{1,3})",
    "(::f{4}:)?169\\.254\\.([0-9]{1,3})\\.([0-9]{1,3})",
    "f[cd][0-9a-f]{2}:.*",
    "fe80:.*",
    "::1",
    "::"};

void WriteNetworkAuditResultsToDisk(const base::Value::Dict& results_dic,
                                    const base::FilePath& path) {
  std::string results;
  JSONFileValueSerializer serializer(path);
  serializer.Serialize(results_dic);

  LOG(INFO) << "Network audit results stored in " << path << std::endl;
}

bool isPrivateURL(const GURL& url) {
  for (const char* regexp : kPrivateIPRegexps) {
    if (RE2::FullMatch(url.host(), regexp)) {
      return true;
    }
  }
  return false;
}

bool PerformNetworkAuditProcess(
    base::Value::List* events,
    const std::vector<std::string>& extra_allowed_prefixes) {
  DCHECK(events);

  bool failed = false;
  events->EraseIf([&failed, &extra_allowed_prefixes](base::Value& event_value) {
    base::Value::Dict* event_dict = event_value.GetIfDict();
    EXPECT_TRUE(event_dict);

    std::optional<int> event_type = event_dict->FindInt("type");
    EXPECT_TRUE(event_type.has_value());

    // Showing these helps determine which URL requests which don't
    // actually hit the network.
    if (static_cast<net::NetLogEventType>(event_type.value()) ==
        net::NetLogEventType::URL_REQUEST_FAKE_RESPONSE_HEADERS_CREATED) {
      return false;
    }

    const auto* source_dict = event_dict->FindDict("source");
    EXPECT_TRUE(source_dict);

    // Consider URL requests only.
    std::optional<int> source_type = source_dict->FindInt("type");
    EXPECT_TRUE(source_type.has_value());

    if (static_cast<net::NetLogSourceType>(source_type.value()) !=
        net::NetLogSourceType::URL_REQUEST) {
      return true;
    }

    // Discard events without URLs in the parameters.
    if (!event_dict->Find("params")) {
      return true;
    }

    const auto* params_dict2 = event_dict->FindDict("params");
    EXPECT_TRUE(params_dict2);

    if (!params_dict2->Find("url")) {
      return true;
    }

    const std::string* url_str = params_dict2->FindString("url");
    EXPECT_TRUE(url_str);

    GURL url(*url_str);
    if (!url.is_valid()) {
      // Network requests to invalid URLs don't pose a threat and can happen in
      // dev-only environments (e.g. building with brave_stats_updater_url="").
      return true;
    }

    if (RE2::FullMatch(url.host(), "[a-z]+")) {
      // Chromium sometimes sends requests to random non-resolvable hosts.
      return true;
    }

    for (const char* protocol : kAllowedUrlProtocols) {
      if (protocol == url.scheme()) {
        return true;
      }
    }

    bool found_prefix = false;
    for (const char* prefix : kAllowedUrlPrefixes) {
      if (!url.spec().rfind(prefix, 0)) {
        found_prefix = true;
        break;
      }
    }

    if (!found_prefix) {
      for (const std::string& prefix : extra_allowed_prefixes) {
        if (!url.spec().rfind(prefix.c_str(), 0)) {
          found_prefix = true;
          break;
        }
      }
    }

    bool found_pattern = false;
    for (const char* pattern : kAllowedUrlPatterns) {
      if (RE2::FullMatch(url.spec(), pattern)) {
        found_pattern = true;
        break;
      }
    }

    if (!found_prefix && !found_pattern) {
      // Check if the URL is a private IP.
      if (isPrivateURL(url)) {
        // Warn but don't fail the audit.
        LOG(WARNING) << "NETWORK AUDIT WARNING:" << url.spec() << std::endl;
        return false;
      }

      LOG(ERROR) << "NETWORK AUDIT FAIL:" << url.spec() << std::endl;
      failed = true;
    }

    return false;
  });

  return !failed;
}
}  // namespace

void VerifyNetworkAuditLog(
    const base::FilePath& net_log_path,
    const base::FilePath& audit_results_path,
    const std::vector<std::string>& extra_allowed_prefixes) {
  // Read the netlog from disk.
  std::string file_contents;
  ASSERT_TRUE(base::ReadFileToString(net_log_path, &file_contents))
      << "Could not read: " << net_log_path;

  // Parse it as JSON.
  auto parsed = base::JSONReader::Read(file_contents);
  ASSERT_TRUE(parsed.has_value());

  // Ensure the root value is a dictionary.
  auto* main = parsed->GetIfDict();
  ASSERT_TRUE(main);

  // Ensure it has a "constants" property.
  auto* constants = main->FindDict("constants");
  ASSERT_TRUE(constants);
  ASSERT_FALSE(constants->empty());

  // Ensure it has an "events" property.
  auto* events = main->FindList("events");
  ASSERT_TRUE(events);
  ASSERT_FALSE(events->empty());

  EXPECT_TRUE(PerformNetworkAuditProcess(events, extra_allowed_prefixes))
      << "network-audit FAILED. Import " << net_log_path.AsUTF8Unsafe()
      << " in chrome://net-internals for more details.";

  // Write results of the audit to disk, useful for further debugging.
  WriteNetworkAuditResultsToDisk(*main, audit_results_path);
  ASSERT_TRUE(base::PathExists(audit_results_path));
}
}  // namespace brave
