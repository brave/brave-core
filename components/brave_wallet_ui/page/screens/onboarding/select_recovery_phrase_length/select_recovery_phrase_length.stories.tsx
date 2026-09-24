// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import { RecoveryPhraseLengths } from '../../../../constants/types'

// Components
import { SelectRecoveryPhraseLength } from './select_recovery_phrase_length'
import {
  WalletPageStory, //
} from '../../../../stories/wrappers/wallet-page-story-wrapper'

export const _SelectRecoveryPhraseLength = () => {
  // State
  const [selectedLength, setSelectedLength] =
    React.useState<RecoveryPhraseLengths>()

  return (
    <WalletPageStory>
      <SelectRecoveryPhraseLength
        selectedLength={selectedLength}
        onSelectedLengthChange={setSelectedLength}
      />
    </WalletPageStory>
  )
}

export default {
  component: _SelectRecoveryPhraseLength,
  title: 'Wallet/Desktop/Screens/Onboarding',
}
