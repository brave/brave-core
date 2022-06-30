// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Routing
import { Redirect, useParams } from 'react-router'

// Redux
import { useDispatch, useSelector } from 'react-redux'

// Actions
import { AccountsTabActions } from '../../../../page/reducers/accounts-tab-reducer'

// Types
import {
  BraveWallet,
  CoinTypesMap,
  WalletState,
  WalletRoutes,
  AccountModalTypes,
  AccountButtonOptionsObjectType
} from '../../../../constants/types'

// Components
import { create } from 'ethereum-blockies'
import { BackButton, Tooltip } from '../../../shared'
import {
  PortfolioAssetItem,
  PortfolioTransactionItem
} from '../..'
import AccountListItemButton from '../../account-list-item/account-list-item-button'

// Utils
import { reduceAddress } from '../../../../utils/reduce-address'
import { getLocale } from '../../../../../common/locale'
import { sortTransactionByDate } from '../../../../utils/tx-utils'
import { AccountButtonOptions } from '../../../../options/account-buttons'

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

// Hooks
import { useBalance, useCopy } from '../../../../common/hooks'

export interface Props {
  goBack: () => void
}

export const Account = (props: Props) => {
  const { goBack } = props

  // routing
  const { id: accountId } = useParams<{ id: string }>()

  // redux
  const dispatch = useDispatch()
  const {
    accounts,
    transactions,
    transactionSpotPrices,
    userVisibleTokensInfo,
    defaultCurrencies,
    networkList
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  // custom hooks
  const getBalance = useBalance(networkList)
  const { copied, copyText } = useCopy()

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
    const chainList = networkList.filter((network) => network.coin === selectedAccount?.coin).map((network) => network.chainId) ?? []
    const list =
      userVisibleTokensInfo.filter((token) => chainList.includes(token?.chainId ?? '') &&
        token.chainId !== BraveWallet.LOCALHOST_CHAIN_ID) ?? []
    if (accountsLocalHost) {
      return [...list, accountsLocalHost]
    }
    return list
  }, [userVisibleTokensInfo, selectedAccount, networkList])

  const nonFungibleTokens = React.useMemo(
    () => accountsTokensList.filter(({ isErc721 }) => isErc721),
    [accountsTokensList]
  )

  const funigbleTokens = React.useMemo(
    () => accountsTokensList.filter(({ isErc721 }) => !isErc721),
    [accountsTokensList]
  )

  const accountButtonOptions = React.useMemo((): AccountButtonOptionsObjectType[] => {
    return AccountButtonOptions.filter((option) => option.id !== 'details' && option.id !== 'remove')
  }, [])

  // methods
  const onCopyToClipboard = React.useCallback(async () => {
    if (selectedAccount) {
      await copyText(selectedAccount.address)
    }
  }, [selectedAccount])

  const onShowAccountsModal = React.useCallback((modalType: AccountModalTypes) => () => {
    dispatch(AccountsTabActions.setShowAccountModal(true))
    dispatch(AccountsTabActions.setAccountModalType(modalType))
    dispatch(AccountsTabActions.setSelectedAccount(selectedAccount))
  }, [selectedAccount])

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
          <Tooltip
            text={getLocale('braveWalletToolTipCopyToClipboard')}
            actionText={getLocale('braveWalletToolTipCopiedToClipboard')}
            isActionVisible={copied}
          >
            <WalletAddress onClick={onCopyToClipboard}>{reduceAddress(selectedAccount.address)}</WalletAddress>
          </Tooltip>
        </WalletInfoLeftSide>
        <AccountButtonsRow>
          {accountButtonOptions.map((option) =>
            <AccountListItemButton
              key={option.id}
              option={option}
              onClick={onShowAccountsModal(option.id as AccountModalTypes)}
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
              key={`${item.contractAddress}-${item.symbol}-${item.chainId}`}
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
