// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { BraveWallet } from '../../../constants/types'
import {
  mockHardwareAccounts //
} from '../../../stories/mock-data/mock-wallet-accounts'

import {
  WalletPageStory //
} from '../../../stories/wrappers/wallet-page-story-wrapper'
import { HardwareWalletAccountsList } from './accounts_list'
import { Meta } from '@storybook/react'

export const HardwareAccountsList = {
  render: () => <WalletPageStory>
    <HardwareWalletAccountsList
      hardwareWallet={'Ledger'}
      accounts={mockHardwareAccounts}
      preAddedHardwareWalletAccounts={[]}
      onLoadMore={function (): void {
        throw new Error('Function not implemented.')
      }}
      selectedDerivationPaths={[]}
      setSelectedDerivationPaths={function (paths: string[]): void {
        throw new Error('Function not implemented.')
      }}
      selectedDerivationScheme={''}
      setSelectedDerivationScheme={function (scheme: string): void {
        throw new Error('Function not implemented.')
      }}
      onAddAccounts={function (): void {
        throw new Error('Function not implemented.')
      }}
      filecoinNetwork={'f'}
      onChangeFilecoinNetwork={function (network: 'f' | 't'): void {
        throw new Error('Function not implemented.')
      }}
      coin={BraveWallet.CoinType.ETH}
    />
  </WalletPageStory>
}

export default {
  title: 'Hardware Wallet Accounts List',
} as Meta<typeof HardwareWalletAccountsList>
