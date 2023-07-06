// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { useHistory, useLocation, useParams } from 'react-router'

// utils
import { getLocale } from '$web-common/locale'
import { keyringIdForNewAccount } from '../../../../utils/account-utils'

// options
import { CreateAccountOptions } from '../../../../options/create-account-options'

// types
import {
  BitcoinNetwork,
  BitcoinNetworkLocaleMapping,
  BitcoinNetworkTypes,
  BraveWallet,
  CreateAccountOptionsType,
  FilecoinNetwork,
  FilecoinNetworkLocaleMapping,
  FilecoinNetworkTypes,
  ImportAccountErrorType,
  WalletRoutes
} from '../../../../constants/types'

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

// selectors
import { WalletSelectors } from '../../../../common/selectors'

// hooks
import { useSafeWalletSelector, useUnsafeWalletSelector } from '../../../../common/hooks/use-safe-selector'

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
  const isFilecoinEnabled = useSafeWalletSelector(WalletSelectors.isFilecoinEnabled)
  const isSolanaEnabled = useSafeWalletSelector(WalletSelectors.isSolanaEnabled)
  const isBitcoinEnabled = useSafeWalletSelector(WalletSelectors.isBitcoinEnabled)
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)

  // state
  const [accountName, setAccountName] = React.useState<string>('')
  const [filecoinNetwork, setFilecoinNetwork] = React.useState<FilecoinNetwork>(BraveWallet.FILECOIN_MAINNET)
  const [bitcoinNetwork, setBitcoinNetwork] = React.useState<BitcoinNetwork>(BraveWallet.BITCOIN_TESTNET)

  // memos
  const createAccountOptions = React.useMemo(() => {
    return CreateAccountOptions({
      isFilecoinEnabled,
      isSolanaEnabled,
      isBitcoinEnabled
    })
  }, [isFilecoinEnabled, isSolanaEnabled, isBitcoinEnabled])

  const selectedAccountType = React.useMemo(() => {
    return createAccountOptions.find((option) => {
      return option.name.toLowerCase() === accountTypeName?.toLowerCase()
    })
  }, [accountTypeName, createAccountOptions])

  const suggestedAccountName = React.useMemo(() => {
    const accountTypeLength = accounts.filter((account) => account.accountId.coin === selectedAccountType?.coin).length + 1
    return `${selectedAccountType?.name} ${getLocale('braveWalletAccount')} ${accountTypeLength}`
  }, [accounts, selectedAccountType])

  const targetKeyringId = React.useMemo(() => {
    if (!selectedAccountType) {
      return
    }
    const network =
      (selectedAccountType.coin === BraveWallet.CoinType.FIL &&
        filecoinNetwork) ||
      (selectedAccountType.coin === BraveWallet.CoinType.BTC &&
        bitcoinNetwork) ||
      undefined

    return keyringIdForNewAccount(selectedAccountType.coin, network)
  }, [selectedAccountType, filecoinNetwork, bitcoinNetwork])

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

  const onChangeBitcoinNetwork = React.useCallback((network: BitcoinNetwork) => {
    setBitcoinNetwork(network)
  }, [])

  const onClickCreateAccount = React.useCallback(() => {
    if (!selectedAccountType) {
      return
    }
    if (targetKeyringId === undefined) {
      return
    }

    dispatch(
      WalletActions.addAccount({
        coin: selectedAccountType.coin,
        keyringId: targetKeyringId,
        accountName
      })
    )
    if (walletLocation.includes(WalletRoutes.Accounts)) {
      history.push(WalletRoutes.Accounts)
    }
  }, [accountName, selectedAccountType, targetKeyringId])

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
            <SelectWrapper>
              <Select value={filecoinNetwork} onChange={onChangeFilecoinNetwork}>
                {FilecoinNetworkTypes.map((network) => {
                  return (
                    <div data-value={network} key={network}>
                      {FilecoinNetworkLocaleMapping[network]}
                    </div>
                  )
                })}
              </Select>
            </SelectWrapper>
          }
          {selectedAccountType?.coin === BraveWallet.CoinType.BTC &&
            <SelectWrapper>
              <Select value={bitcoinNetwork} onChange={onChangeBitcoinNetwork}>
                {BitcoinNetworkTypes.map((network) => {
                  return (
                    <div data-value={network} key={network}>
                      {BitcoinNetworkLocaleMapping[network]}
                    </div>
                  )
                })}
              </Select>
            </SelectWrapper>
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
          createAccountOptions={createAccountOptions}
          buttonText={getLocale('braveWalletAddAccountCreate')}
          onSelectAccountType={pickNewAccountType}
        />
      }
    </PopupModal>
  )
}

export default CreateAccountModal
