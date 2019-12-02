/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_INFOBARS_BRAVE_INFOBAR_DELEGATE_H_
#define BRAVE_BROWSER_INFOBARS_BRAVE_INFOBAR_DELEGATE_H_

#include "components/infobars/core/infobar_delegate.h"

enum BraveInfoBarIdentifier {
  BRAVE_CONFIRM_P3A_INFOBAR_DELEGATE = 500,
  CRYPTO_WALLETS_INFOBAR_DELEGATE = 501,
  WAYBACK_MACHINE_INFOBAR_DELEGATE = 502,
};

#endif  // BRAVE_BROWSER_INFOBARS_BRAVE_INFOBAR_DELEGATE_H_
