/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_TALK_COMMON_BRAVE_TALK_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_TALK_COMMON_BRAVE_TALK_UTILS_H_

class GURL;

namespace brave_talk {

bool IsAllowedHost(const GURL& url);
bool IsJSAPIEnabled();

}  // namespace brave_talk

#endif  // BRAVE_COMPONENTS_BRAVE_TALK_COMMON_BRAVE_TALK_UTILS_H_
