/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BINANCE_BROWSER_BINANCE_PROTOCOL_HANDLER_H_
#define BRAVE_COMPONENTS_BINANCE_BROWSER_BINANCE_PROTOCOL_HANDLER_H_

#include <string>

#include "chrome/browser/external_protocol/external_protocol_handler.h"
#include "chrome/browser/profiles/profile.h"

namespace contribute {

void HandleContributeProtocol(const GURL& url,
                           content::WebContents::OnceGetter web_contents_getter,
                           ui::PageTransition page_transition,
                           bool has_user_gesture);

bool IsContributeProtocol(const GURL& url);

}  // namespace contribute

#endif  // BRAVE_COMPONENTS_CONTRIBUTE_BROWSER_CONTRIBUTE_PROTOCOL_HANDLER_H_
