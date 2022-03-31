/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_BRAVE_SERVICES_KEY_HELPER_H_
#define BRAVE_COMMON_BRAVE_SERVICES_KEY_HELPER_H_

class GURL;

namespace brave {

bool ShouldAddBraveServicesKeyHeader(const GURL& url);

}  // namespace brave

#endif  // BRAVE_COMMON_BRAVE_SERVICES_KEY_HELPER_H_
