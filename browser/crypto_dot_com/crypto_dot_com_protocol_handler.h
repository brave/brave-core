/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_CRYPTO_DOT_COM_CRYPTO_DOT_COM_PROTOCOL_HANDLER_H_
#define BRAVE_BROWSER_CRYPTO_DOT_COM_CRYPTO_DOT_COM_PROTOCOL_HANDLER_H_

#include "base/optional.h"
#include "content/public/browser/web_contents.h"
#include "ui/base/page_transition_types.h"

class GURL;

namespace crypto_dot_com {

void HandleCryptoDotComProtocol(
    const GURL& url,
    content::WebContents::OnceGetter web_contents_getter,
    ui::PageTransition page_transition,
    bool has_user_gesture,
    const base::Optional<url::Origin>& initiator);

bool IsCryptoDotComProtocol(const GURL& url);

}  // namespace crypto_dot_com

#endif  // BRAVE_BROWSER_CRYPTO_DOT_COM_CRYPTO_DOT_COM_PROTOCOL_HANDLER_H_
