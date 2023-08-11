// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Selectors
import { WalletSelectors } from '../../../../../common/selectors'
import { useUnsafeWalletSelector } from '../../../../../common/hooks/use-safe-selector'

// Assets
import PersonIcon from '../../assets/person.svg'
import CaratDownIcon from '../../assets/carat-down-icon.svg'

// Hooks
import { useOnClickOutside } from '../../../../../common/hooks/useOnClickOutside'
import {
  useGetFVMAddressQuery,
  useGetSelectedChainQuery //
} from '../../../../../common/slices/api.slice'

// Components
import { AccountListItem } from '../account-list-item/account-list-item'

// Styled Components
import { ButtonIcon, ArrowIcon, DropDown, SelectorButton } from './account-selector.style'

import { skipToken } from '@reduxjs/toolkit/query/react'
import { BraveWallet } from '../../../../../constants/types'

interface Props {
  asset: BraveWallet.BlockchainToken | undefined,
  onSelectAddress: (value: string) => void
  disabled: boolean
}

const ACCOUNT_SELECTOR_BUTTON_ID = 'account-selector-button-id'

export const AccountSelector = (props: Props) => {
  const { asset, onSelectAddress, disabled } = props

  // Selectors
  const accounts = useUnsafeWalletSelector(WalletSelectors.accounts)
  const selectedAccount = useUnsafeWalletSelector(WalletSelectors.selectedAccount)

  // Queries
  const { data: selectedNetwork } = useGetSelectedChainQuery()

  // State
  const [showAccountSelector, setShowAccountSelector] = React.useState<boolean>(false)

  // Refs
  const accountSelectorRef = React.useRef<HTMLDivElement>(null)

  // Methods
  const toggleShowAccountSelector = React.useCallback(() => {
    setShowAccountSelector(prev => !prev)
  }, [])

  const handleOnSelectAccount = React.useCallback((address: string) => {
    setShowAccountSelector(false)
    onSelectAddress(address)
  }, [onSelectAddress])

  const isFVMAccount = React.useCallback(
    (account) =>
      (selectedNetwork?.chainId === BraveWallet.FILECOIN_ETHEREUM_MAINNET_CHAIN_ID &&
        account.accountId.keyringId === BraveWallet.KeyringId.kFilecoin) ||
      (selectedNetwork?.chainId === BraveWallet.FILECOIN_ETHEREUM_TESTNET_CHAIN_ID &&
        account.accountId.keyringId === BraveWallet.KeyringId.kFilecoinTestnet),
    [selectedNetwork]);

  // Memos
  const accountsByNetwork = React.useMemo(() => {
    if (!selectedNetwork || !selectedAccount) {
      return []
    }

    if (selectedAccount.accountId.coin === BraveWallet.CoinType.FIL) {
      const filecoinAccounts = accounts.filter((account) =>
        (account.accountId.keyringId === selectedAccount.accountId.keyringId))
      const fevmAccounts = accounts.filter((account) =>
        (account.accountId.coin === BraveWallet.CoinType.ETH))
      return filecoinAccounts.concat(fevmAccounts)
    }

    // TODO(apaymyshev): for bitcoin should allow sending to my account, but
    // from different keyring (i.e. segwit -> taproot)
    // https://github.com/brave/brave-browser/issues/29262
    return accounts.filter(
      (account) =>
        account.accountId.keyringId === selectedAccount.accountId.keyringId ||
        (asset?.contractAddress === "" && isFVMAccount(account)))
  }, [accounts, selectedNetwork, selectedAccount, asset])

  const evmAddressesforFVMTranslation = React.useMemo(() =>
  accountsByNetwork
    .filter(account => account.accountId.coin === BraveWallet.CoinType.ETH)
    .map(account => account.accountId.address),
  [accountsByNetwork])

  const { data: fvmTranslatedAddresses } = useGetFVMAddressQuery(
    selectedNetwork && evmAddressesforFVMTranslation.length
    ? {
        coin: selectedNetwork.coin,
        addresses: evmAddressesforFVMTranslation,
        isMainNet: selectedNetwork.chainId === BraveWallet.FILECOIN_MAINNET
      }
    : skipToken
  )

  // Hooks
  useOnClickOutside(
    accountSelectorRef,
    () => setShowAccountSelector(false),
    showAccountSelector,
    ACCOUNT_SELECTOR_BUTTON_ID
  )

  return (
    <>
      <SelectorButton disabled={disabled} id={ACCOUNT_SELECTOR_BUTTON_ID} onClick={toggleShowAccountSelector}>
        <ButtonIcon id={ACCOUNT_SELECTOR_BUTTON_ID} icon={PersonIcon} size={16} />
        <ArrowIcon id={ACCOUNT_SELECTOR_BUTTON_ID} icon={CaratDownIcon} size={12} isOpen={showAccountSelector} />
      </SelectorButton>
      {showAccountSelector &&
        <DropDown ref={accountSelectorRef}>
          {accountsByNetwork.map((account) =>
            <AccountListItem
              key={account.address}
              onClick={handleOnSelectAccount}
              address={account.address}
              name={account.name}
              accountAlias={fvmTranslatedAddresses?.[account.accountId.address]}
            />
          )}
        </DropDown>
      }
    </>
  )
}

export default AccountSelector
