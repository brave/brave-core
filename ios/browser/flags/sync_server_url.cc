// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/flags/sync_server_url.h"

#include <algorithm>
#include <string>
#include <string_view>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/brave_sync/buildflags.h"
#include "components/sync/base/command_line_switches.h"
#include "components/webui/flags/flags_storage.h"
#include "net/base/url_util.h"
#include "url/gurl.h"
#include "url/url_constants.h"

namespace brave_sync {
namespace {

GURL ParseSyncServerUrl(std::string_view input) {
  input = base::TrimWhitespaceASCII(input, base::TRIM_ALL);
  const size_t scheme_end = input.find(':');
  if (scheme_end == std::string_view::npos ||
      !input.substr(scheme_end).starts_with("://") ||
      std::ranges::any_of(input, [](unsigned char c) {
        return c <= ' ' || c == '\\' || c == 0x7f;
      })) {
    return {};
  }

  const auto authority_and_path = input.substr(scheme_end + 3);
  const auto authority =
      authority_and_path.substr(0, authority_and_path.find_first_of("/?#"));
  if (authority.empty() || authority.find('@') != std::string_view::npos) {
    return {};
  }

  GURL url(input);
  if (!url.is_valid() || !url.has_host() || url.has_username() ||
      url.has_password() || url.has_query() || url.has_ref() ||
      (!url.SchemeIs(url::kHttpsScheme) &&
       !(url.SchemeIs(url::kHttpScheme) && net::IsLocalhost(url)))) {
    return {};
  }
  return url;
}

}  // namespace

void SetSyncServerUrlFlag(flags_ui::FlagsStorage* flags_storage,
                          const base::ListValue& args) {
  if (args.size() != 2) {
    return;
  }
  const auto* name = args[0].GetIfString();
  const auto* value = args[1].GetIfString();
  if (!name || *name != kSyncServerUrlFlagName || !value) {
    return;
  }
  flags_storage->SetStringFlag(kSyncServerUrlFlagName, *value);
  flags_storage->CommitPendingWrites();
}

void ApplySyncServerUrlFromFlags(const flags_ui::FlagsStorage& flags_storage,
                                 base::CommandLine& command_line) {
  // Keep explicit launch and developer settings overrides.
  if (command_line.HasSwitch(syncer::kSyncServiceURL)) {
    return;
  }

  if (flags_storage.GetFlags().contains(kSyncServerUrlFlagName)) {
    const std::string value =
        flags_storage.GetStringFlag(kSyncServerUrlFlagName);
    if (!value.empty()) {
      const GURL url = ParseSyncServerUrl(value);
      if (url.is_valid()) {
        command_line.AppendSwitchASCII(syncer::kSyncServiceURL, url.spec());
        return;
      }
      LOG(WARNING) << "Invalid Brave Sync server URL in brave://flags.";
    }
  }

  command_line.AppendSwitchASCII(syncer::kSyncServiceURL,
                                 BUILDFLAG(BRAVE_SYNC_ENDPOINT));
}

}  // namespace brave_sync
