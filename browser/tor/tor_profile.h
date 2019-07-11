/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_TOR_TOR_PROFILE_H_
#define BRAVE_BROWSER_TOR_TOR_PROFILE_H_

class Profile;

namespace tor {

bool IsTorProfile(const Profile* profile);

}   // namespace tor

#endif  // BRAVE_BROWSER_TOR_TOR_PROFILE_H_
