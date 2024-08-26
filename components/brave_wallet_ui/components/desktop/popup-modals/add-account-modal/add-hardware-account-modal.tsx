// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, useParams } from 'react-router'

// utils
import { getLocale } from '$web-common/locale'

// options
import { CreateAccountOptions } from '../../../../options/create-account-options'

// types
import {
  CreateAccountOptionsType,
  WalletRoutes
} from '../../../../constants/types'

// components
import { DividerLine } from '../../../extension/divider/index'
import PopupModal from '..'
import {
  HardwareWalletConnect //
} from '../../hardware-wallet-connect/hardware_wallet_connect'
import { SelectAccountType } from './select-account-type/select-account-type'

// style
import { StyledWrapper } from './style'

// selectors
import { WalletSelectors } from '../../../../common/selectors'

// hooks
import { useSafeWalletSelector } from '../../../../common/hooks/use-safe-selector'

interface Params {
  accountTypeName: string
}

interface Props {
  onSelectAccountType: (accountType: CreateAccountOptionsType) => () => void
}

export const AddHardwareAccountModal = ({ onSelectAccountType }: Props) => {
  // routing
  const history = useHistory()
  const { accountTypeName } = useParams<Params>()

  const isBitcoinLedgerEnabled = useSafeWalletSelector(
    WalletSelectors.isBitcoinLedgerEnabled
  )

  // memos
  const createAccountOptions = React.useMemo(() => {
    return CreateAccountOptions({
      isBitcoinEnabled: isBitcoinLedgerEnabled,
      isZCashEnabled: false // No zcash hardware accounts by now.
    })
  }, [isBitcoinLedgerEnabled])

  const selectedAccountType: CreateAccountOptionsType | undefined =
    React.useMemo(() => {
      return createAccountOptions.find((option) => {
        return option.name.toLowerCase() === accountTypeName?.toLowerCase()
      })
    }, [createAccountOptions, accountTypeName])

  // methods
  const closeModal = React.useCallback(() => {
    history.push(WalletRoutes.Accounts)
  }, [history])

  // render
  return (
    <PopupModal
      title={getLocale('braveWalletAddAccountImportHardware')}
      onClose={closeModal}
    >
      <DividerLine />

      {selectedAccountType && (
        <StyledWrapper>
          <HardwareWalletConnect
            selectedAccountType={selectedAccountType}
            onSuccess={closeModal}
          />
        </StyledWrapper>
      )}

      {!selectedAccountType && (
        <SelectAccountType
          createAccountOptions={createAccountOptions}
          onSelectAccountType={onSelectAccountType}
          buttonText={getLocale('braveWalletAddAccountConnect')}
        />
      )}
    </PopupModal>
  )
}

export default AddHardwareAccountModal
