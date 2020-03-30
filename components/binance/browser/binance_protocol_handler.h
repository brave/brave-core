/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BINANCE_PROTOCOL_HANDLER_H_
#define BRAVE_COMPONENTS_BINANCE_PROTOCOL_HANDLER_H_

#include <string>

#include "chrome/browser/external_protocol/external_protocol_handler.h"
#include "chrome/browser/profiles/profile.h"

namespace binance {

void HandleBinanceProtocol(const GURL& url,
                           content::WebContents::OnceGetter web_contents_getter,
                           ui::PageTransition page_transition,
                           bool has_user_gesture);

bool IsBinanceProtocol(const GURL& url);

}  // namespace binance

#endif  // BRAVE_COMPONENTS_BINANCE_PROTOCOL_HANDLER_H_
