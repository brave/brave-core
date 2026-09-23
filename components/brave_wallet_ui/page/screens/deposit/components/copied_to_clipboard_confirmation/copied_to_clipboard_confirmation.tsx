// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '$web-common/locale'

// style
import { GreenCheckmark } from '$wallet/components/shared/style'
import { CopiedToClipboardContainer } from './copied_to_clipboard_confirmation.style'

export const CopiedToClipboardConfirmation: React.FC<{}> = () => {
  return (
    <CopiedToClipboardContainer>
      <GreenCheckmark />
      <p>{getLocale(S.BRAVE_WALLET_COPIED_TO_CLIPBOARD)}</p>
    </CopiedToClipboardContainer>
  )
}
