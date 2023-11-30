/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <optional>

#include "base/check.h"
#include "brave/components/constants/brave_services_key.h"
#include "brave/components/widevine/static_buildflags.h"
#include "components/update_client/protocol_serializer.h"

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
#include "brave/components/widevine/constants.h"
#endif

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
#define MakeProtocolRequest MakeProtocolRequest_ChromiumImpl
#endif

#define BuildUpdateCheckExtraRequestHeaders \
  BuildUpdateCheckExtraRequestHeaders_ChromiumImpl
#include "src/components/update_client/protocol_serializer.cc"
#undef BuildUpdateCheckExtraRequestHeaders

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)
#undef MakeProtocolRequest
#endif

namespace update_client {

base::flat_map<std::string, std::string> BuildUpdateCheckExtraRequestHeaders(
    const std::string& prod_id,
    const base::Version& browser_version,
    const std::vector<std::string>& ids,
    bool is_foreground) {
  auto headers = BuildUpdateCheckExtraRequestHeaders_ChromiumImpl(
      prod_id, browser_version, ids, is_foreground);
  headers.insert({"BraveServiceKey", BUILDFLAG(BRAVE_SERVICES_KEY)});
  return headers;
}

#if BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)

protocol_request::Request MakeProtocolRequest(
    const bool is_machine,
    const std::string& session_id,
    const std::string& prod_id,
    const std::string& browser_version,
    const std::string& channel,
    const std::string& os_long_name,
    const std::string& download_preference,
    std::optional<bool> domain_joined,
    const base::flat_map<std::string, std::string>& additional_attributes,
    const base::flat_map<std::string, std::string>& updater_state_attributes,
    std::vector<protocol_request::App> apps) {
  // As of this writing, App is not copy constructible. This implies that
  // every invocation of MakeProtocolRequest wraps the `apps` parameter with
  // std::move. This further implies that it is safe for us to do the same when
  // we call MakeProtocolRequest_ChromiumImpl below. The following assert checks
  // that upstream hasn't changed in this regard, and thus that our use of
  // std::move(apps) is still safe:
  static_assert(!std::is_copy_constructible<protocol_request::App>::value,
                "App is copy constructible.");

  std::string fake_architecture;

  // additional_attributes is const, so we need to create a copy:
  base::flat_map<std::string, std::string> additional_attributes_copy =
      additional_attributes;
  if (additional_attributes_copy.contains(kFakeArchitectureAttribute)) {
    fake_architecture = additional_attributes_copy[kFakeArchitectureAttribute];
    additional_attributes_copy.erase(kFakeArchitectureAttribute);
  }

  protocol_request::Request request = MakeProtocolRequest_ChromiumImpl(
      is_machine, session_id, prod_id, browser_version, channel, os_long_name,
      download_preference, domain_joined, additional_attributes_copy,
      updater_state_attributes, std::move(apps));

  if (!fake_architecture.empty()) {
    request.arch = request.os.arch = fake_architecture;
  }
  return request;
}

#endif  // BUILDFLAG(WIDEVINE_ARM64_DLL_FIX)

}  // namespace update_client
