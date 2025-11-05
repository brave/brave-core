// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/common/extensions/brave_extensions_client.h"

#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "brave/components/skus/common/skus_utils.h"
#include "components/component_updater/component_updater_switches.h"
#include "extensions/common/extension_urls.h"

namespace extensions {

namespace {

std::string ParseUpdateUrlHost(std::string options) {
  std::vector<std::string> flags = base::SplitString(
      options, ",", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  for (const auto& flag : flags) {
    std::vector<std::string> values = base::SplitString(
        flag, "=", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

    if (values.size() != 2) {
      continue;
    }

    std::string name = base::ToLowerASCII(values[0]);
    std::string value = values[1];

    if (name == "url-source") {
      return value;
    }
  }

  return "";
}

}  // namespace

BraveExtensionsClient::BraveExtensionsClient() = default;

void BraveExtensionsClient::InitializeWebStoreUrls(
    base::CommandLine* command_line) {
  std::string url_from_switch = ParseUpdateUrlHost(
      command_line->GetSwitchValueASCII(switches::kComponentUpdater));
  if (!url_from_switch.empty()) {
    webstore_update_url_ = GURL(url_from_switch);
  } else {
    // If no URL is provided via command line, use the default webstore URL.
    // This ensures webstore_update_url_ is always valid.
    webstore_update_url_ = extension_urls::GetDefaultWebstoreUpdateUrl();
  }
  ChromeExtensionsClient::InitializeWebStoreUrls(command_line);
}

bool BraveExtensionsClient::IsScriptableURL(const GURL& url,
                                            std::string* error) const {
  if (skus::IsSafeOrigin(url)) {
    if (error) {
      *error = "This site is protected and cannot be scripted.";
    }
    return false;
  }
  return ChromeExtensionsClient::IsScriptableURL(url, error);
}

const GURL& BraveExtensionsClient::GetWebstoreUpdateURL() const {
  // webstore_update_url_ should always be valid after InitializeWebStoreUrls()
  // is called. If it's somehow invalid, return the default URL by calling
  // the parent class method which will return a valid URL.
  if (!webstore_update_url_.is_valid()) {
    return ChromeExtensionsClient::GetWebstoreUpdateURL();
  }
  return webstore_update_url_;
}

}  // namespace extensions
