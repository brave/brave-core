// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

mangle((root) => {
  // Brave disables email verification, which would leave the card with only
  // its heading. Hidden rather than removed, since the element's `$` declares
  // #emailSharedMenu.
  const emailVerificationCard = root
    .getElementById('emailSharedMenu')
    ?.closest('.card')
  if (!emailVerificationCard) {
    throw new Error(
      `[Settings] Contact info page: couldn't find the email verification card`,
    )
  }
  emailVerificationCard.setAttribute('hidden', '')
})
