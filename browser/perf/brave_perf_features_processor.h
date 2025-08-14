// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_PERF_BRAVE_PERF_FEATURES_PROCESSOR_H_
#define BRAVE_BROWSER_PERF_BRAVE_PERF_FEATURES_PROCESSOR_H_

class Profile;

namespace perf {

// Handlers for --enable-brave-features-for-perf-testing switch.
// They are split into two because the first is invoked from
// BraveProfileManager::InitProfileUserPrefs and cannot instantiate services
// that rely on identity manager because ProfileImpl::OnLocaleReady expects
// identity manager not to be initialized before browser context services are
// created. The second one is invoked from
// BraveProfileManager::DoFinalInitForServices.
void MaybeEnableBraveFeaturesPrefsForPerfTesting(Profile* profile);
void MaybeEnableBraveFeaturesServicesAndComponentsForPerfTesting(
    Profile* profile);

}  // namespace perf

#endif  // BRAVE_BROWSER_PERF_BRAVE_PERF_FEATURES_PROCESSOR_H_
