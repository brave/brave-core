/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/chromium_src/chrome/browser/profiles/profile.h"

#include "base/strings/string_util.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/constants/brave_constants.h"
#include "brave/components/tor/tor_constants.h"
#include "chrome/browser/policy/profile_policy_connector.h"
#include "components/search_engines/search_engine_choice/search_engine_choice_utils.h"

#define BRAVE_ALLOWS_BROWSER_WINDOWS *this == TorID() ||

#define IsIncognitoProfile IsIncognitoProfile_ChromiumImpl
#define IsPrimaryOTRProfile IsPrimaryOTRProfile_ChromiumImpl
#include <chrome/browser/profiles/profile.cc>
#undef IsIncognitoProfile
#undef IsPrimaryOTRProfile
#undef BRAVE_ALLOWS_BROWSER_WINDOWS

namespace {
const char kSearchBackupResultsOTRProfileIDPrefix[] =
    "SearchBackupResults::OTR";
const char kCodeSandboxOTRProfileIDPrefix[] = "CodeSandbox::OTR";
}  // namespace

// static
const Profile::OTRProfileID Profile::OTRProfileID::TorID() {
  return OTRProfileID(tor::kTorProfileID);
}

bool Profile::IsTor() const {
  return IsOffTheRecord() && GetOTRProfileID() == OTRProfileID::TorID();
}

bool Profile::IsAIChatAgent() const {
#if BUILDFLAG(ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE)
  if (!ai_chat::features::IsAIChatAgentProfileEnabled()) {
    return false;
  }
  return GetPath().BaseName().value() == brave::kAIChatAgentProfileDir;
#else
  return false;
#endif
}

bool Profile::IsIncognitoProfile() const {
  if (IsTor())
    return true;
  return IsIncognitoProfile_ChromiumImpl();
}

// Tor profile should behave like primary OTR profile used in private window
bool Profile::IsPrimaryOTRProfile() const {
  if (IsTor())
    return true;
  return IsPrimaryOTRProfile_ChromiumImpl();
}

Profile::OTRProfileID
Profile::OTRProfileID::CreateUniqueForSearchBackupResults() {
  return CreateUnique(kSearchBackupResultsOTRProfileIDPrefix);
}

bool Profile::OTRProfileID::IsSearchBackupResults() const {
  return base::StartsWith(profile_id_, kSearchBackupResultsOTRProfileIDPrefix,
                          base::CompareCase::SENSITIVE);
}

Profile::OTRProfileID Profile::OTRProfileID::CreateUniqueForCodeSandbox() {
  return CreateUnique(kCodeSandboxOTRProfileIDPrefix);
}

bool Profile::OTRProfileID::IsCodeSandbox() const {
  return base::StartsWith(profile_id_, kCodeSandboxOTRProfileIDPrefix,
                          base::CompareCase::SENSITIVE);
}

// This is to avoid a circular dep on chrome/browser in the Factory for
// the brave_origin keyed service.
namespace brave_origin {
policy::PolicyService* GetPolicyServiceFromProfile(Profile* profile) {
  if (auto* connector = profile->GetProfilePolicyConnector()) {
    return connector->policy_service();
  }
  return nullptr;
}
}  // namespace brave_origin
