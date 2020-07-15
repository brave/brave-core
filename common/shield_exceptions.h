/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMMON_SHIELD_EXCEPTIONS_H_
#define BRAVE_COMMON_SHIELD_EXCEPTIONS_H_

class GURL;

namespace brave {

bool IsUAWhitelisted(const GURL& gurl);

}  // namespace brave

#endif  // BRAVE_COMMON_SHIELD_EXCEPTIONS_H_
