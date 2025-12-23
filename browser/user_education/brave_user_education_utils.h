/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_USER_EDUCATION_BRAVE_USER_EDUCATION_UTILS_H_
#define BRAVE_BROWSER_USER_EDUCATION_BRAVE_USER_EDUCATION_UTILS_H_

class UserEducationService;

namespace brave {

// Suppresses "New" badges for specific features by manipulating badge data to
// exceed policy limits. This ensures badges never appear for features we want
// to suppress. Called during profile initialization.
void SuppressNewBadgesForFeatures(UserEducationService* service);

}  // namespace brave

#endif  // BRAVE_BROWSER_USER_EDUCATION_BRAVE_USER_EDUCATION_UTILS_H_
