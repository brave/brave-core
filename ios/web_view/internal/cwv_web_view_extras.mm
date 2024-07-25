/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/ios/web_view/public/cwv_web_view_extras.h"

#include "ios/web/common/user_agent.h"

const CWVUserAgentType CWVUserAgentTypeNone =
    static_cast<CWVUserAgentType>(web::UserAgentType::NONE);
const CWVUserAgentType CWVUserAgentTypeAutomatic =
    static_cast<CWVUserAgentType>(web::UserAgentType::AUTOMATIC);
const CWVUserAgentType CWVUserAgentTypeMobile =
    static_cast<CWVUserAgentType>(web::UserAgentType::MOBILE);
const CWVUserAgentType CWVUserAgentTypeDesktop =
    static_cast<CWVUserAgentType>(web::UserAgentType::DESKTOP);
