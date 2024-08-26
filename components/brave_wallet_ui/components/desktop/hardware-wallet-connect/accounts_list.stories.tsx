// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  mockAccountsFromDevice //
} from '../../../stories/mock-data/mock-wallet-accounts'

import {
  WalletPageStory //
} from '../../../stories/wrappers/wallet-page-story-wrapper'
import { HardwareWalletAccountsList } from './accounts_list'
import { Meta } from '@storybook/react'
import {
  DerivationScheme,
  EthLedgerDeprecatedHardwareImportScheme,
  EthLedgerLegacyHardwareImportScheme,
  EthLedgerLiveHardwareImportScheme
} from '../../../common/hardware/types'

export const HardwareAccountsList = {
  render: () => (
    <WalletPageStory>
      <HardwareWalletAccountsList
        accounts={mockAccountsFromDevice.map((a, i) => {
          return {
            ...a,
            shouldAddToWallet: i === 0,
            alreadyInWallet: i !== 0
          }
        })}
        onLoadMore={function (): void {}}
        currentHardwareImportScheme={EthLedgerLiveHardwareImportScheme}
        supportedSchemes={[
          EthLedgerLiveHardwareImportScheme,
          EthLedgerLegacyHardwareImportScheme,
          EthLedgerDeprecatedHardwareImportScheme
        ]}
        setHardwareImportScheme={function (scheme: DerivationScheme): void {}}
        onAddAccounts={function (): void {}}
        onAccountChecked={function (path: string, checked: boolean): void {}}
      />
    </WalletPageStory>
  )
}

export default {
  title: 'Hardware Wallet Accounts List',
  component: HardwareWalletAccountsList
} as Meta<typeof HardwareWalletAccountsList>
