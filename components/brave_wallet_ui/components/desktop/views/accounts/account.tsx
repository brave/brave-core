// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Redirect, useParams } from 'react-router'
import { useDispatch, useSelector } from 'react-redux'
import { create } from 'ethereum-blockies'

import {
  AccountSettingsNavTypes,
  UpdateAccountNamePayloadType,
  BraveWallet,
  CoinTypesMap,
  WalletState,
  PageState,
  WalletRoutes
} from '../../../../constants/types'

// utils
import { reduceAddress } from '../../../../utils/reduce-address'
import { getLocale } from '../../../../../common/locale'
import { sortTransactionByDate } from '../../../../utils/tx-utils'

// Styled Components
import {
  StyledWrapper,
  SubDivider,
  Button,
  TopRow,
  WalletInfoRow,
  WalletAddress,
  WalletName,
  AccountCircle,
  WalletInfoLeftSide,
  QRCodeIcon,
  EditIcon,
  SubviewSectionTitle,
  TransactionPlaceholderContainer
} from './style'
import { TransactionPlaceholderText, Spacer } from '../portfolio/style'

// Components
import { BackButton } from '../../../shared'
import { AccountSettingsModal } from '../../popup-modals/account-settings-modal/account-settings-modal'
import { PortfolioTransactionItem } from '../../portfolio-transaction-item/index'
import { PortfolioAssetItem } from '../../portfolio-asset-item/index'
import { CopyTooltip } from '../../../shared/copy-tooltip/copy-tooltip'

// Hooks
import { useBalance } from '../../../../common/hooks/balance'

// Actions
import { WalletPageActions } from '../../../../page/actions'

export interface Props {
  onViewPrivateKey: (address: string, isDefault: boolean, coin: BraveWallet.CoinType) => void
  onDoneViewingPrivateKey: () => void
  toggleNav: () => void
  onUpdateAccountName: (payload: UpdateAccountNamePayloadType) => { success: boolean }
  goBack: () => void
}

export const Account = ({
  goBack,
  onViewPrivateKey,
  onDoneViewingPrivateKey,
  toggleNav,
  onUpdateAccountName
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
  const privateKey = useSelector(({ page }: { page: PageState }) => page.privateKey)

  // state
  const [showEditModal, setShowEditModal] = React.useState<boolean>(false)
  const [editTab, setEditTab] = React.useState<AccountSettingsNavTypes>('details')

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
    () =>
      accountsTokensList.filter(({ isErc721 }) => isErc721)
        .filter((token) => getBalance(selectedAccount, token) !== '0'),
    [accountsTokensList, selectedAccount, getBalance]
  )

  const funigbleTokens = React.useMemo(
    () => accountsTokensList.filter(({ isErc721 }) => !isErc721),
    [accountsTokensList]
  )

  const onShowEditModal = React.useCallback(() => {
    setShowEditModal(true)
  }, [])

  const onCloseEditModal = React.useCallback(() => {
    setShowEditModal(false)
    setEditTab('details')
  }, [])

  const onRemoveAccount = React.useCallback((address: string, hardware: boolean, coin: BraveWallet.CoinType) => {
    if (hardware) {
      dispatch(WalletPageActions.removeHardwareAccount({ address, coin }))
      return
    }
    dispatch(WalletPageActions.removeImportedAccount({ address, coin }))
  }, [])

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
          <Button onClick={onShowEditModal}>
            <QRCodeIcon />
          </Button>
        </WalletInfoLeftSide>
        <Button onClick={onShowEditModal}>
          <EditIcon />
        </Button>
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

      {showEditModal && selectedAccount &&
        <AccountSettingsModal
          title={getLocale('braveWalletAccount')}
          account={selectedAccount}
          onClose={onCloseEditModal}
          onUpdateAccountName={onUpdateAccountName}
          onChangeTab={setEditTab}
          onToggleNav={toggleNav}
          onRemoveAccount={onRemoveAccount}
          onViewPrivateKey={onViewPrivateKey}
          onDoneViewingPrivateKey={onDoneViewingPrivateKey}
          privateKey={privateKey || ''}
          tab={editTab}
          hideNav={false}
        />
      }
    </StyledWrapper>
  )
}

export default Account
