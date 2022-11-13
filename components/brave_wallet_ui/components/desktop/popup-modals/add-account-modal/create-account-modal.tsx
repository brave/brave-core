// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch, useSelector } from 'react-redux'
import { useHistory, useLocation, useParams } from 'react-router'

// utils
import { getLocale } from '$web-common/locale'

// options
import { CreateAccountOptions } from '../../../../options/create-account-options'

// types
import {
  BraveWallet,
  CreateAccountOptionsType,
  WalletRoutes,
  WalletState,
  ImportAccountErrorType
} from '../../../../constants/types'
import { FilecoinNetworkTypes, FilecoinNetworkLocaleMapping, FilecoinNetwork } from '../../../../common/hardware/types'

// actions
import { WalletPageActions } from '../../../../page/actions'
import { WalletActions } from '../../../../common/actions'

// components
import { DividerLine, NavButton } from '../../../../components/extension'
import PopupModal from '..'
import { SelectAccountType } from './select-account-type'
import { Select } from 'brave-ui/components'

// style
import {
  Input,
  StyledWrapper,
  SelectWrapper
} from './style'

interface Params {
  accountTypeName: string
}

export const CreateAccountModal = () => {
  // routing
  const history = useHistory()
  const { pathname: walletLocation } = useLocation()
  const { accountTypeName } = useParams<Params>()

  // redux
  const dispatch = useDispatch()
  const isFilecoinEnabled = useSelector(({ wallet }: { wallet: WalletState }) => wallet.isFilecoinEnabled)
  const isSolanaEnabled = useSelector(({ wallet }: { wallet: WalletState }) => wallet.isSolanaEnabled)
  const accounts = useSelector(({ wallet }: { wallet: WalletState }) => wallet.accounts)

  // state
  const [accountName, setAccountName] = React.useState<string>('')
  const [filecoinNetwork, setFilecoinNetwork] = React.useState<FilecoinNetwork>('f')

  // memos
  const selectedAccountType: CreateAccountOptionsType | undefined = React.useMemo(() => {
    if (!accountTypeName) {
      return undefined
    }
    return CreateAccountOptions(isFilecoinEnabled, isSolanaEnabled).find(option => {
      return option.name.toLowerCase() === accountTypeName.toLowerCase()
    })
  }, [accountTypeName])

  const suggestedAccountName = React.useMemo(() => {
    const accountTypeLength = accounts.filter((account) => account.coin === selectedAccountType?.coin).length + 1
    return `${selectedAccountType?.name} ${getLocale('braveWalletAccount')} ${accountTypeLength}`
  }, [accounts, selectedAccountType])

  // methods
  const setImportAccountError = React.useCallback((hasError: ImportAccountErrorType) => {
    dispatch(WalletPageActions.setImportAccountError(hasError))
  }, [])

  const onClickClose = React.useCallback(() => {
    setImportAccountError(undefined)
    history.push(WalletRoutes.Accounts)
  }, [setImportAccountError])

  const handleAccountNameChanged = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setAccountName(event.target.value)
    setImportAccountError(undefined)
  }, [setImportAccountError])

  const onChangeFilecoinNetwork = React.useCallback((network: FilecoinNetwork) => {
    setFilecoinNetwork(network)
  }, [])

  const onClickCreateAccount = React.useCallback(() => {
    const created =
       (selectedAccountType?.coin === BraveWallet.CoinType.FIL)
          ? dispatch(WalletActions.addFilecoinAccount({ accountName, network: filecoinNetwork }))
          : dispatch(WalletActions.addAccount({ accountName, coin: selectedAccountType?.coin || BraveWallet.CoinType.ETH }))
    if (created) {
      if (walletLocation.includes(WalletRoutes.Accounts)) {
        history.push(WalletRoutes.Accounts)
      }
    }
  }, [accountName, selectedAccountType, filecoinNetwork])

  const handleKeyDown = React.useCallback((event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter') {
      onClickCreateAccount()
    }
  }, [onClickCreateAccount])

  const pickNewAccountType = React.useCallback((option: CreateAccountOptionsType) => () => {
    history.push(WalletRoutes.CreateAccountModal
      .replace(':accountTypeName?', option.name.toLowerCase())
    )
  }, [])

  // effects
  React.useEffect(() => {
    setAccountName(suggestedAccountName)
  }, [suggestedAccountName])

  // computed
  const isDisabled = accountName === ''
  const modalTitle = selectedAccountType
    ? getLocale('braveWalletCreateAccount').replace('$1', selectedAccountType.name)
    : getLocale('braveWalletCreateAccountButton')

  // render
  return (
    <PopupModal title={modalTitle} onClose={onClickClose}>
      <DividerLine />
      {selectedAccountType &&
        <StyledWrapper>
          {selectedAccountType?.coin === BraveWallet.CoinType.FIL &&
            <>
            <SelectWrapper>
              <Select value={filecoinNetwork} onChange={onChangeFilecoinNetwork}>
                {FilecoinNetworkTypes.map((network, index) => {
                  const networkLocale = FilecoinNetworkLocaleMapping[network]
                  return (
                    <div data-value={network} key={index}>
                      {networkLocale}
                    </div>
                  )
                })}
              </Select>
            </SelectWrapper>
          </>
          }

          <Input
            value={accountName}
            placeholder={getLocale('braveWalletAddAccountPlaceholder')}
            onKeyDown={handleKeyDown}
            onChange={handleAccountNameChanged}
            autoFocus={true}
          />

          <NavButton
            onSubmit={onClickCreateAccount}
            disabled={isDisabled}
            text={getLocale('braveWalletCreateAccountButton')}
            buttonType='primary'
          />
        </StyledWrapper>
      }

      {!selectedAccountType &&
        <SelectAccountType
          buttonText={getLocale('braveWalletAddAccountCreate')}
          onSelectAccountType={pickNewAccountType}
        />
      }
    </PopupModal>
  )
}

export default CreateAccountModal
