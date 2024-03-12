// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useParams } from 'react-router'

// utils
import {
  BraveWallet,
  CreateAccountOptionsType
} from '../../../../constants/types'
import { CreateAccountOptions } from '../../../../options/create-account-options'

// components
import { OnboardingContentLayout } from '../components/onboarding-content-layout/onboarding-content-layout'
import { HardwareButton } from './components/hardware-button'

// styles
import { VerticalSpace } from '../../../../components/shared/style'
import { Divider } from './components/hardware-button.style'
import { HardwareVendor } from '../../../../common/api/hardware_keyrings'
import {
  HardwareDerivationScheme,
  LedgerDerivationPaths,
  SolDerivationPaths
} from '../../../../common/hardware/types'
import { HardwareWalletDerivationPathsMapping } from '../../../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect/types'
import HardwareWalletConnect from '../../../../components/desktop/popup-modals/add-account-modal/hardware-wallet-connect'

interface Params {
  accountTypeName: string
}

export const OnboardingSelectWalletDevice = () => {
  // routing
  // const history = useHistory()
  const { accountTypeName } = useParams<Params>()

  // state
  const [selectedHardwareWallet, setSelectedHardwareWallet] =
    React.useState<HardwareVendor>()

  // memos
  const accountOptions = React.useMemo(() => {
    return CreateAccountOptions({
      isBitcoinEnabled: false, // No bitcoin hardware accounts by now.
      isZCashEnabled: false // No zcash hardware accounts by now.
    })
  }, [])

  const selectedAccountType: CreateAccountOptionsType | undefined =
    React.useMemo(() => {
      return accountOptions.find((option) => {
        return option.name.toLowerCase() === accountTypeName?.toLowerCase()
      })
    }, [accountOptions, accountTypeName])

  const [selectedDerivationScheme, setSelectedDerivationScheme] =
    React.useState<HardwareDerivationScheme>(
      selectedAccountType?.coin === BraveWallet.CoinType.SOL
        ? SolDerivationPaths.Default
        : LedgerDerivationPaths.LedgerLive
    )

  // methods
  const selectVendor = React.useCallback((vendor: HardwareVendor) => {
    const derivationPathsEnum = HardwareWalletDerivationPathsMapping[vendor]
    setSelectedDerivationScheme(
      Object.values(derivationPathsEnum)[0] as HardwareDerivationScheme
    )
    setSelectedHardwareWallet(vendor)
  }, [])

  const onSelectLedger = React.useCallback(() => {
    if (selectedHardwareWallet !== BraveWallet.LEDGER_HARDWARE_VENDOR) {
      // setConnectionError(undefined)
    }

    selectVendor(BraveWallet.LEDGER_HARDWARE_VENDOR)
  }, [selectedHardwareWallet, selectVendor])

  const onSelectTrezor = React.useCallback(() => {
    if (selectedHardwareWallet !== BraveWallet.TREZOR_HARDWARE_VENDOR) {
      // setConnectionError(undefined)
    }

    selectVendor(BraveWallet.TREZOR_HARDWARE_VENDOR)
  }, [selectedHardwareWallet, selectVendor])

  return (
    <OnboardingContentLayout title='Select your hardware wallet device'>
      {selectedHardwareWallet && selectedAccountType ? (
        <HardwareWalletConnect
          selectedAccountType={selectedAccountType}
          selectedHardwareWallet={selectedHardwareWallet}
          derivationScheme={selectedDerivationScheme}
          onSuccess={() => {}}
        />
      ) : (
        <>
          <VerticalSpace space='98px' />
          <HardwareButton
            title='Ledger'
            description='Connect your Ledger device to Brave Wallet'
            onClick={onSelectLedger}
          />
          <Divider />
          <HardwareButton
            title='Trezor'
            description='Connect your Trezor device to Brave Wallet'
            onClick={onSelectTrezor}
          />
        </>
      )}
    </OnboardingContentLayout>
  )
}
