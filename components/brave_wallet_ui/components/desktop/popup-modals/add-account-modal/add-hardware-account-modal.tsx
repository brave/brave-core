// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { useHistory, useParams } from 'react-router'

// utils
import { getLocale } from '$web-common/locale'

// options
import { CreateAccountOptions } from '../../../../options/create-account-options'

// types
import {
  CreateAccountOptionsType,
  WalletRoutes,
  ImportAccountErrorType
} from '../../../../constants/types'

// actions
import { WalletPageActions } from '../../../../page/actions'

// components
import { DividerLine } from '../../../extension'
import PopupModal from '..'
import { HardwareWalletConnect } from './hardware-wallet-connect'
import { SelectAccountType } from './select-account-type/select-account-type'

// style
import {
  StyledWrapper
} from './style'

// hooks
import { WalletSelectors } from '../../../../common/selectors'

// selectors
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

  // redux
  const dispatch = useDispatch()
  const isFilecoinEnabled = useSafeWalletSelector(WalletSelectors.isFilecoinEnabled)
  const isSolanaEnabled = useSafeWalletSelector(WalletSelectors.isSolanaEnabled)

  // memos
  const createAccountOptions = React.useMemo(() => {
    return CreateAccountOptions({
      isFilecoinEnabled,
      isSolanaEnabled,
      isBitcoinEnabled: false // No bitcoin hardware accounts by now.
    })
  }, [isFilecoinEnabled, isSolanaEnabled])

  const selectedAccountType: CreateAccountOptionsType | undefined = React.useMemo(() => {
    return createAccountOptions.find((option) => {
      return option.name.toLowerCase() === accountTypeName?.toLowerCase()
    })
  }, [createAccountOptions, accountTypeName])

  // methods
  const setImportError = React.useCallback((hasError: ImportAccountErrorType) => {
    dispatch(WalletPageActions.setImportAccountError(hasError))
  }, [])

  const closeModal = React.useCallback(() => {
    setImportError(undefined)
    history.push(WalletRoutes.Accounts)
  }, [setImportError])

  // render
  return (
    <PopupModal
      title={getLocale('braveWalletAddAccountImportHardware')}
      onClose={closeModal}
    >

      <DividerLine />

      {selectedAccountType &&
        <StyledWrapper>
          <HardwareWalletConnect
            selectedAccountType={selectedAccountType}
            onSuccess={closeModal}
          />
        </StyledWrapper>
      }

      {!selectedAccountType &&
        <SelectAccountType
          createAccountOptions={createAccountOptions}
          onSelectAccountType={onSelectAccountType}
          buttonText={getLocale('braveWalletAddAccountConnect')}
        />
      }
    </PopupModal>
  )
}

export default AddHardwareAccountModal
