// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { mockAccount } from '../../../../common/constants/mocks'
import { WalletPageStory } from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { AccountSettingsModal } from './account-settings-modal'

export const _AccountSettingsModalPrivateKey: React.FC = () => {
  return <WalletPageStory
    walletStateOverride={{ selectedAccount: mockAccount }}
    accountTabStateOverride={{ selectedAccount: mockAccount, accountModalType: 'privateKey' }}
  >
    <AccountSettingsModal />
  </WalletPageStory>
}

export const _AccountSettingsModalDeposit: React.FC = () => {
  return <WalletPageStory
    walletStateOverride={{ selectedAccount: mockAccount }}
    accountTabStateOverride={{ selectedAccount: mockAccount, accountModalType: 'deposit' }}
  >
    <AccountSettingsModal />
  </WalletPageStory>
}

export const _AccountSettingsModalEdit: React.FC = () => {
  return <WalletPageStory
    walletStateOverride={{ selectedAccount: mockAccount }}
    accountTabStateOverride={{ selectedAccount: mockAccount, accountModalType: 'edit' }}
  >
    <AccountSettingsModal />
  </WalletPageStory>
}

export const _AccountSettingsModalRemove: React.FC = () => {
  return <WalletPageStory
    walletStateOverride={{ selectedAccount: mockAccount }}
    accountTabStateOverride={{ selectedAccount: mockAccount, accountModalType: 'remove' }}
  >
    <AccountSettingsModal />
  </WalletPageStory>
}

export default _AccountSettingsModalPrivateKey
