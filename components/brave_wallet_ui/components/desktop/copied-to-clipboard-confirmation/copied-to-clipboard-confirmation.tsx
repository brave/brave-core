// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// utils
import { getLocale } from '../../../../common/locale'

// style
import { GreenCheckmark } from '../../shared/style'
import { CopiedToClipboardContainer } from './copied-to-clipboard-confirmation.style'

export const CopiedToClipboardConfirmation: React.FC<{}> = () => {
  return <CopiedToClipboardContainer>
    <GreenCheckmark />
    <p>{getLocale('braveWalletCopiedToClipboard')}</p>
  </CopiedToClipboardContainer>
}

export default CopiedToClipboardConfirmation
