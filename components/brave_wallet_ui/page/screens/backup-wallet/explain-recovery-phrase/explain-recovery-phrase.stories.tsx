// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import WalletPageStory from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { RecoveryPhraseExplainer } from './explain-recovery-phrase'

export const _BackupRecoveryPhraseExplainer = () => {
  return <WalletPageStory>
    <RecoveryPhraseExplainer />
  </WalletPageStory>
}

_BackupRecoveryPhraseExplainer.story = {
  name: 'Explain Backup Recovery Phrase'
}

export default _BackupRecoveryPhraseExplainer
