/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PROFILES_PROFILE_UTIL_H_
#define BRAVE_BROWSER_PROFILES_PROFILE_UTIL_H_

class Profile;

namespace brave {

bool IsTorProfile(const Profile* profile);

// This function is similar to Profile::IsGuestSession. It is used to get
// around the bug(?) in OffTheRecordProfileImpl::Init() code that calls
// set_is_guest_profile on off-the-record profile only after calling
// BrowserContextDependencyManager::CreateBrowserContextServices. Because of
// this, in search_engine_provider_service_factory.cc (for example), in
// InitializeSearchEngineProviderServiceIfNeeded we can't correctly identify
// the guest profile by calling IsGuestSession and have to use this function.
bool IsGuestProfile(Profile* profile);

}  // namespace brave

#endif  // BRAVE_BROWSER_PROFILES_PROFILE_UTIL_H_
