/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_USER_EDUCATION_BRAVE_USER_EDUCATION_UTILS_H_
#define BRAVE_BROWSER_USER_EDUCATION_BRAVE_USER_EDUCATION_UTILS_H_

class UserEducationService;

namespace brave {

// Suppresses user education elements (New badges and IPH promos) for features
// that Brave doesn't want to promote. Called during profile initialization.
void SuppressUserEducation(UserEducationService* service);

}  // namespace brave

#endif  // BRAVE_BROWSER_USER_EDUCATION_BRAVE_USER_EDUCATION_UTILS_H_
