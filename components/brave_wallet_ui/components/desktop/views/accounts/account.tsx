// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Redirect, useParams } from 'react-router'
import {
  useDispatch,
  useSelector
} from 'react-redux'
import { create } from 'ethereum-blockies'

import {
  BraveWallet,
  CoinTypesMap,
  WalletState,
  WalletRoutes,
  AccountButtonOptionsObjectType
} from '../../../../constants/types'

// utils
import { reduceAddress } from '../../../../utils/reduce-address'
import { getLocale } from '../../../../../common/locale'
import { sortTransactionByDate } from '../../../../utils/tx-utils'

// Styled Components
import {
  StyledWrapper,
  SubDivider,
  TopRow,
  WalletInfoRow,
  WalletAddress,
  WalletName,
  AccountCircle,
  WalletInfoLeftSide,
  SubviewSectionTitle,
  TransactionPlaceholderContainer,
  AccountButtonsRow
} from './style'
import { TransactionPlaceholderText, Spacer } from '../portfolio/style'

// Components
import { BackButton } from '../../../shared'
import { PortfolioTransactionItem } from '../../portfolio-transaction-item/index'
import { PortfolioAssetItem } from '../../portfolio-asset-item/index'
import { CopyTooltip } from '../../../shared/copy-tooltip/copy-tooltip'
import { AccountListItemOptionButton } from '../../account-list-item/account-list-item-option-button'

// options
import { AccountButtonOptions } from '../../../../options/account-list-button-options'

// Hooks
import { useBalance } from '../../../../common/hooks/balance'

// Actions
import { getFilecoinKeyringIdFromNetwork } from '../../../../utils/network-utils'
import { AccountsTabActions } from '../../../../page/reducers/accounts-tab-reducer'

export interface Props {
  goBack: () => void
}

export const Account = ({
  goBack
}: Props) => {
  // routing
  const { id: accountId } = useParams<{ id: string }>()

  // redux
  const dispatch = useDispatch()
  const accounts = useSelector(({ wallet }: { wallet: WalletState }) => wallet.accounts)
  const transactions = useSelector(({ wallet }: { wallet: WalletState }) => wallet.transactions)
  const transactionSpotPrices = useSelector(({ wallet }: { wallet: WalletState }) => wallet.transactionSpotPrices)
  const userVisibleTokensInfo = useSelector(({ wallet }: { wallet: WalletState }) => wallet.userVisibleTokensInfo)
  const defaultCurrencies = useSelector(({ wallet }: { wallet: WalletState }) => wallet.defaultCurrencies)
  const networkList = useSelector(({ wallet }: { wallet: WalletState }) => wallet.networkList)

  // custom hooks
  const getBalance = useBalance(networkList)

  // memos
  const selectedAccount = React.useMemo(() => {
    return accounts.find(({ address }) =>
      address.toLowerCase() === accountId?.toLowerCase()
    )
  }, [accounts, accountId])

  const orb = React.useMemo(() => {
    if (selectedAccount) {
      return create({ seed: selectedAccount.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
    }
  }, [selectedAccount])

  const transactionList = React.useMemo(() => {
    if (selectedAccount?.address && transactions[selectedAccount.address]) {
      return sortTransactionByDate(transactions[selectedAccount.address], 'descending')
    } else {
      return []
    }
  }, [selectedAccount, transactions])

  const accountsTokensList = React.useMemo(() => {
    if (!selectedAccount) {
      return []
    }
    // Since LOCALHOST's chainId is shared between coinType's
    // this check will make sure we are returning the correct
    // LOCALHOST asset for each account.
    const coinName = CoinTypesMap[selectedAccount?.coin ?? 0]
    const localHostCoins = userVisibleTokensInfo.filter((token) => token.chainId === BraveWallet.LOCALHOST_CHAIN_ID)
    const accountsLocalHost = localHostCoins.find((token) => token.symbol.toUpperCase() === coinName)
    const chainList = networkList.filter((network) => network.coin === selectedAccount?.coin &&
      (network.coin !== BraveWallet.CoinType.FIL || getFilecoinKeyringIdFromNetwork(network) === selectedAccount?.keyringId)).map((network) => network.chainId) ?? []
    const list =
      userVisibleTokensInfo.filter((token) => chainList.includes(token?.chainId ?? '') &&
        token.chainId !== BraveWallet.LOCALHOST_CHAIN_ID) ?? []
    if (accountsLocalHost && (selectedAccount.keyringId !== BraveWallet.FILECOIN_KEYRING_ID)) {
      return [...list, accountsLocalHost]
    }
    return list
  }, [userVisibleTokensInfo, selectedAccount, networkList])

  const nonFungibleTokens = React.useMemo(
    () =>
      accountsTokensList.filter(({ isErc721 }) => isErc721)
        .filter((token) => getBalance(selectedAccount, token) !== '0'),
    [accountsTokensList, selectedAccount, getBalance]
  )

  const funigbleTokens = React.useMemo(
    () => accountsTokensList.filter(({ isErc721 }) => !isErc721),
    [accountsTokensList]
  )

  const isHardwareWallet: boolean = React.useMemo(() => {
    return selectedAccount?.accountType === 'Trezor' || selectedAccount?.accountType === 'Ledger'
  }, [selectedAccount])

  const buttonOptions = React.useMemo((): AccountButtonOptionsObjectType[] => {
    const filteredButtonOptions = AccountButtonOptions.filter((option: AccountButtonOptionsObjectType) => option.id !== 'details')
    // We are not able to remove a Primary account so we filter out this option.
    if (selectedAccount?.accountType === 'Primary') {
      return filteredButtonOptions.filter((option: AccountButtonOptionsObjectType) => option.id !== 'remove')
    }
    // We are not able to fetch Private Keys for a Hardware account so we filter out this option.
    if (isHardwareWallet) {
      return filteredButtonOptions.filter((option: AccountButtonOptionsObjectType) => option.id !== 'privateKey')
    }
    return filteredButtonOptions
  }, [selectedAccount, isHardwareWallet])

  // methods
  const onRemoveAccount = React.useCallback(() => {
    if (selectedAccount) {
      dispatch(AccountsTabActions.setAccountToRemove({ address: selectedAccount.address, hardware: isHardwareWallet, coin: selectedAccount.coin, name: selectedAccount.name }))
    }
  }, [selectedAccount, isHardwareWallet, dispatch])

  const onClickButtonOption = React.useCallback((option: AccountButtonOptionsObjectType) => () => {
    if (option.id === 'remove') {
      onRemoveAccount()
      return
    }
    dispatch(AccountsTabActions.setShowAccountModal(true))
    dispatch(AccountsTabActions.setAccountModalType(option.id))
    dispatch(AccountsTabActions.setSelectedAccount(selectedAccount))
  }, [onRemoveAccount, dispatch])

  // redirect (asset not found)
  if (!selectedAccount) {
    return <Redirect to={WalletRoutes.Accounts} />
  }

  // render
  return (
    <StyledWrapper>
      <TopRow>
        <BackButton onSubmit={goBack} />
      </TopRow>

      <WalletInfoRow>
        <WalletInfoLeftSide>
          <AccountCircle orb={orb} />
          <WalletName>{selectedAccount.name}</WalletName>
          <CopyTooltip text={selectedAccount.address}>
            <WalletAddress>{reduceAddress(selectedAccount.address)}</WalletAddress>
          </CopyTooltip>
        </WalletInfoLeftSide>
        <AccountButtonsRow>
          {buttonOptions.map((option) =>
            <AccountListItemOptionButton
              key={option.id}
              option={option}
              onClick={onClickButtonOption(option)}
            />
          )}
        </AccountButtonsRow>
      </WalletInfoRow>

      <SubviewSectionTitle>{getLocale('braveWalletAccountsAssets')}</SubviewSectionTitle>

      <SubDivider />

      {funigbleTokens.map((item) =>
        <PortfolioAssetItem
          spotPrices={transactionSpotPrices}
          defaultCurrencies={defaultCurrencies}
          key={`${item.contractAddress}-${item.symbol}-${item.chainId}`}
          assetBalance={getBalance(selectedAccount, item)}
          networks={networkList}
          token={item}
        />
      )}

      <Spacer />

      {nonFungibleTokens?.length !== 0 &&
        <>
          <Spacer />
          <SubviewSectionTitle>{getLocale('braveWalletTopNavNFTS')}</SubviewSectionTitle>
          <SubDivider />
          {nonFungibleTokens?.map((item) =>
            <PortfolioAssetItem
              spotPrices={transactionSpotPrices}
              networks={networkList}
              defaultCurrencies={defaultCurrencies}
              key={`${item.contractAddress}-${item.symbol}-${item.chainId}-${item.tokenId}`}
              assetBalance={getBalance(selectedAccount, item)}
              token={item}
            />
          )}
          <Spacer />
        </>
      }

      <Spacer />

      <SubviewSectionTitle>{getLocale('braveWalletTransactions')}</SubviewSectionTitle>

      <SubDivider />

      {transactionList.length !== 0 ? (
        <>
          {transactionList.map((transaction) =>
            <PortfolioTransactionItem
              key={transaction?.id}
              transaction={transaction}
              account={selectedAccount}
              accounts={accounts}
              displayAccountName={false}
            />
          )}
        </>
      ) : (
        <TransactionPlaceholderContainer>
          <TransactionPlaceholderText>{getLocale('braveWalletTransactionPlaceholder')}</TransactionPlaceholderText>
        </TransactionPlaceholderContainer>
      )}
    </StyledWrapper>
  )
}

export default Account
