/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SEARCH_ENGINES_BRAVE_PREPOPULATED_ENGINES_VERSION_H_
#define BRAVE_COMPONENTS_SEARCH_ENGINES_BRAVE_PREPOPULATED_ENGINES_VERSION_H_

namespace TemplateURLPrepopulateData {

// ****************************************************************************
// IMPORTANT! Make sure to bump this value if you add, remove, or make changes
// to the engines in brave_prepopulated_engines.h/cc.
// ****************************************************************************

// The version is important to increment because Chromium will cache the list
// of search engines that are shown. When the version is incremented, Chromium
// will conditionally merge changes from the new version of the list.
//
// For more info, see:
// ComputeMergeEnginesRequirements in components/search_engines/util.cc;

inline constexpr int kBraveCurrentDataVersion = 32;

// DO NOT CHANGE THIS ONE. Used for backfilling kBraveDefaultSearchVersion.
inline constexpr int kBraveFirstTrackedDataVersion = 6;

}  // namespace TemplateURLPrepopulateData

#endif  // BRAVE_COMPONENTS_SEARCH_ENGINES_BRAVE_PREPOPULATED_ENGINES_VERSION_H_
