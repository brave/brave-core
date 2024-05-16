// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory, useLocation, useParams } from 'react-router'
import Input, { InputEventDetail } from '@brave/leo/react/input'
import Dropdown from '@brave/leo/react/dropdown'
import {
  SelectItemEventDetail //
} from '@brave/leo/types/src/components/menu/menu.svelte'

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
  WalletRoutes,
  ZCashNetwork,
  ZCashNetworkLocaleMapping,
  ZCashNetworkTypes
} from '../../../../constants/types'

// components
import { DividerLine } from '../../../../components/extension/divider/index'
import PopupModal from '..'
import { SelectAccountType } from './select-account-type'

// style
import {
  SubmitButtonWrapper,
  CreateAccountStyledWrapper,
  ErrorText
} from './style'

// selectors
import { WalletSelectors } from '../../../../common/selectors'

// hooks
import {
  useSafeWalletSelector //
} from '../../../../common/hooks/use-safe-selector'
import { useAccountsQuery } from '../../../../common/slices/api.slice.extra'
import { useAddAccountMutation } from '../../../../common/slices/api.slice'
import { LeoSquaredButton } from '../../../shared/style'

interface Params {
  accountTypeName: string
}

export const CreateAccountModal = () => {
  // routing
  const history = useHistory()
  const { pathname: walletLocation } = useLocation()
  const { accountTypeName } = useParams<Params>()

  // redux
  const isBitcoinEnabled = useSafeWalletSelector(
    WalletSelectors.isBitcoinEnabled
  )
  const isZCashEnabled = useSafeWalletSelector(WalletSelectors.isZCashEnabled)

  // queries
  const { accounts } = useAccountsQuery()

  // mutations
  const [addAccount] = useAddAccountMutation()

  // state
  const [accountName, setAccountName] = React.useState<string>('')
  const [filecoinNetwork, setFilecoinNetwork] = React.useState<FilecoinNetwork>(
    BraveWallet.FILECOIN_MAINNET
  )
  const [bitcoinNetwork, setBitcoinNetwork] = React.useState<BitcoinNetwork>(
    BraveWallet.BITCOIN_MAINNET
  )
  const [zcashNetwork, setZCashNetwork] = React.useState<ZCashNetwork>(
    BraveWallet.Z_CASH_MAINNET
  )

  // memos
  const createAccountOptions = React.useMemo(() => {
    return CreateAccountOptions({
      isBitcoinEnabled,
      isZCashEnabled
    })
  }, [isBitcoinEnabled, isZCashEnabled])

  const selectedAccountType = React.useMemo(() => {
    return createAccountOptions.find((option) => {
      return option.name.toLowerCase() === accountTypeName?.toLowerCase()
    })
  }, [accountTypeName, createAccountOptions])

  const suggestedAccountName = React.useMemo(() => {
    const accountTypeLength =
      accounts.filter(
        (account) => account.accountId.coin === selectedAccountType?.coin
      ).length + 1
    return `${
      selectedAccountType?.name //
    } ${getLocale('braveWalletSubviewAccount')} ${
      //
      accountTypeLength
    }`
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
      (selectedAccountType.coin === BraveWallet.CoinType.ZEC && zcashNetwork) ||
      undefined

    return keyringIdForNewAccount(selectedAccountType.coin, network)
  }, [selectedAccountType, filecoinNetwork, bitcoinNetwork, zcashNetwork])

  // computed
  const isAccountNameTooLong =
    accountName.length > BraveWallet.ACCOUNT_NAME_MAX_CHARACTER_LENGTH
  const isDisabled = accountName === '' || isAccountNameTooLong
  const modalTitle = selectedAccountType
    ? getLocale('braveWalletCreateAccount').replace(
        '$1',
        selectedAccountType.name
      )
    : getLocale('braveWalletCreateAccountButton')

  // methods
  const onClickClose = React.useCallback(() => {
    history.push(WalletRoutes.Accounts)
  }, [history])

  const handleAccountNameChanged = React.useCallback(
    (detail: InputEventDetail) => {
      setAccountName(detail.value)
    },
    []
  )

  const onChangeFilecoinNetwork = React.useCallback(
    (detail: SelectItemEventDetail) => {
      setFilecoinNetwork(detail.value as FilecoinNetwork)
    },
    []
  )

  const onChangeBitcoinNetwork = React.useCallback(
    (detail: SelectItemEventDetail) => {
      setBitcoinNetwork(detail.value as BitcoinNetwork)
    },
    []
  )

  const onChangeZCashNetwork = React.useCallback(
    (detail: SelectItemEventDetail) => {
      setZCashNetwork(detail.value as ZCashNetwork)
    },
    []
  )

  const onClickCreateAccount = React.useCallback(async () => {
    if (!selectedAccountType || targetKeyringId === undefined || isDisabled) {
      return
    }

    await addAccount({
      coin: selectedAccountType.coin,
      keyringId: targetKeyringId,
      accountName
    })

    if (walletLocation.includes(WalletRoutes.Accounts)) {
      history.push(WalletRoutes.Accounts)
    }
  }, [
    accountName,
    addAccount,
    history,
    isDisabled,
    selectedAccountType,
    targetKeyringId,
    walletLocation
  ])

  const handleKeyDown = React.useCallback(
    (detail: InputEventDetail) => {
      if ((detail.innerEvent as unknown as KeyboardEvent).key === 'Enter') {
        onClickCreateAccount()
      }
    },
    [onClickCreateAccount]
  )

  const pickNewAccountType = React.useCallback(
    (option: CreateAccountOptionsType) => () => {
      history.push(
        WalletRoutes.CreateAccountModal.replace(
          ':accountTypeName?',
          option.name.toLowerCase()
        )
      )
    },
    [history]
  )

  // effects
  React.useEffect(() => {
    setAccountName(suggestedAccountName)
  }, [suggestedAccountName])

  // render
  return (
    <PopupModal
      title={modalTitle}
      onClose={onClickClose}
    >
      <DividerLine />
      {selectedAccountType && (
        <CreateAccountStyledWrapper>
          {selectedAccountType?.coin === BraveWallet.CoinType.FIL && (
            <Dropdown
              value={filecoinNetwork}
              onChange={onChangeFilecoinNetwork}
            >
              <div slot='label'>
                {getLocale('braveWalletAllowAddNetworkNetworkPanelTitle')}
              </div>

              <div slot='value'>
                {FilecoinNetworkLocaleMapping[filecoinNetwork]}
              </div>

              {FilecoinNetworkTypes.map((network, index) => {
                return (
                  <leo-option
                    key={index}
                    value={network}
                  >
                    {FilecoinNetworkLocaleMapping[network]}
                  </leo-option>
                )
              })}
            </Dropdown>
          )}

          {selectedAccountType?.coin === BraveWallet.CoinType.BTC && (
            <Dropdown
              value={bitcoinNetwork}
              onChange={onChangeBitcoinNetwork}
            >
              <div slot='label'>
                {getLocale('braveWalletAllowAddNetworkNetworkPanelTitle')}
              </div>

              <div slot='value'>
                {BitcoinNetworkLocaleMapping[bitcoinNetwork]}
              </div>

              {BitcoinNetworkTypes.map((network) => {
                return (
                  <leo-option
                    key={network}
                    value={network}
                  >
                    {BitcoinNetworkLocaleMapping[network]}
                  </leo-option>
                )
              })}
            </Dropdown>
          )}

          {selectedAccountType?.coin === BraveWallet.CoinType.ZEC && (
            <Dropdown
              value={zcashNetwork}
              onChange={onChangeZCashNetwork}
            >
              <div slot='label'>
                {getLocale('braveWalletAllowAddNetworkNetworkPanelTitle')}
              </div>

              <div slot='value'>{ZCashNetworkLocaleMapping[zcashNetwork]}</div>

              {ZCashNetworkTypes.map((network) => {
                return (
                  <leo-option
                    key={network}
                    value={network}
                  >
                    {ZCashNetworkLocaleMapping[network]}
                  </leo-option>
                )
              })}
            </Dropdown>
          )}

          <Input
            value={accountName}
            placeholder={getLocale('braveWalletAddAccountPlaceholder')}
            onInput={handleAccountNameChanged}
            onKeyDown={handleKeyDown}
            showErrors={isDisabled}
          >
            {
              // Label
              getLocale('braveWalletAddAccountPlaceholder')
            }
            <div slot='errors'>
              <ErrorText>
                {isAccountNameTooLong
                  ? getLocale('braveWalletAccountNameTooLongError').replace(
                      '$1',
                      BraveWallet.ACCOUNT_NAME_MAX_CHARACTER_LENGTH.toString()
                    )
                  : ''}
              </ErrorText>
            </div>
            <div slot='extra'>
              {isAccountNameTooLong ? (
                <ErrorText>
                  {accountName.length}/
                  {BraveWallet.ACCOUNT_NAME_MAX_CHARACTER_LENGTH}
                </ErrorText>
              ) : (
                <span>
                  {accountName.length}/
                  {BraveWallet.ACCOUNT_NAME_MAX_CHARACTER_LENGTH}
                </span>
              )}
            </div>
          </Input>

          <SubmitButtonWrapper>
            <LeoSquaredButton
              onClick={onClickCreateAccount}
              isDisabled={isDisabled}
              kind='filled'
            >
              {getLocale('braveWalletCreateAccountButton')}
            </LeoSquaredButton>
          </SubmitButtonWrapper>
        </CreateAccountStyledWrapper>
      )}

      {!selectedAccountType && (
        <SelectAccountType
          createAccountOptions={createAccountOptions}
          buttonText={getLocale('braveWalletAddAccountCreate')}
          onSelectAccountType={pickNewAccountType}
        />
      )}
    </PopupModal>
  )
}

export default CreateAccountModal
