// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { mockAccount } from '../../../../common/constants/mocks'
import { WalletPageStory } from '../../../../stories/wrappers/wallet-page-story-wrapper'
import { AccountSettingsModal } from './account-settings-modal'
import {
  RemoveAccountModal //
} from '../confirm-password-modal/remove-account-modal'
import { Meta } from '@storybook/react'

export const AccountSettingsModalPrivateKey = {
  render: () => <WalletPageStory
    accountTabStateOverride={{
      selectedAccount: mockAccount,
      accountModalType: 'privateKey'
    }}
    apiOverrides={{ selectedAccountId: mockAccount.accountId }}
  >
    <AccountSettingsModal />
  </WalletPageStory>
}

export const AccountSettingsModalDeposit = {
  render: () => <WalletPageStory
    apiOverrides={{ selectedAccountId: mockAccount.accountId }}
    accountTabStateOverride={{
      selectedAccount: mockAccount,
      accountModalType: 'deposit'
    }}
  >
    <AccountSettingsModal />
  </WalletPageStory>
}

export const _AccountSettingsModalEdit = {
  render: () => <WalletPageStory
    apiOverrides={{ selectedAccountId: mockAccount.accountId }}
    accountTabStateOverride={{
      selectedAccount: mockAccount,
      accountModalType: 'edit'
    }}
  >
    <AccountSettingsModal />
  </WalletPageStory>
}

export const AccountSettingsModalRemove = {
  render: () => <WalletPageStory
    apiOverrides={{ selectedAccountId: mockAccount.accountId }}
    accountTabStateOverride={{
      selectedAccount: mockAccount,
      accountModalType: 'remove',
      accountToRemove: mockAccount
    }}
  >
    <RemoveAccountModal />
  </WalletPageStory>
}

export default {
  component: AccountSettingsModal,
} as Meta<typeof AccountSettingsModal>
