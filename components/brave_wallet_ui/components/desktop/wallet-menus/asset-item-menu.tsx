// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import { BraveWallet } from '../../../constants/types'

// Selectors
import {
  useSafeWalletSelector,
  useSafeUISelector,
} from '../../../common/hooks/use-safe-selector'
import { WalletSelectors, UISelectors } from '../../../common/selectors'

// Queries
import {
  useGetAvailableShieldedAccountQuery,
  useUpdateUserAssetVisibleMutation, //
} from '../../../common/slices/api.slice'
import { useAccountsQuery } from '../../../common/slices/api.slice.extra'

// Hooks
import {
  useMultiChainSellAssets, //
} from '../../../common/hooks/use-multi-chain-sell-assets'
import {
  useFindBuySupportedToken, //
} from '../../../common/hooks/use-multi-chain-buy-assets'
import { useRoute } from '../../../common/hooks/use_route'

// Utils
import { getLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'
import {
  makeDepositFundsRoute,
  makeFundWalletRoute,
  makeSendRoute,
  makeSwapOrBridgeRoute,
} from '../../../utils/routes-utils'
import {
  getAssetIdKey,
  getDoesCoinSupportSwap,
  getDoesCoinSupportBridge,
  isShieldedToken,
} from '../../../utils/asset-utils'

// Components
import {
  SellAssetModal, //
} from '../popup-modals/sell-asset-modal/sell-asset-modal'

// Styled Components
import { ButtonMenu } from './wellet-menus.style'

interface Props {
  asset: BraveWallet.BlockchainToken
  assetBalance: string
  account?: BraveWallet.AccountInfo
  onClickEditToken?: () => void
}

export const AssetItemMenu = (props: Props) => {
  const { asset, assetBalance, account, onClickEditToken } = props

  // State
  const [showSellModal, setShowSellModal] = React.useState<boolean>(false)

  // Selectors
  const isZCashShieldedTransactionsEnabled = useSafeWalletSelector(
    WalletSelectors.isZCashShieldedTransactionsEnabled,
  )
  const isIOS = useSafeUISelector(UISelectors.isIOS)

  // Mutations
  const [updateUserAssetVisible] = useUpdateUserAssetVisibleMutation()

  // Queries
  const { accounts } = useAccountsQuery()
  const zcashAccountIds = accounts
    .filter((account) => account.accountId.coin === BraveWallet.CoinType.ZEC)
    .map((account) => account.accountId)

  const { data: availableShieldedAccountData } =
    useGetAvailableShieldedAccountQuery(
      asset.coin === BraveWallet.CoinType.ZEC
        && isZCashShieldedTransactionsEnabled
        && zcashAccountIds
        ? zcashAccountIds
        : skipToken,
    )

  const shieldedAccount = React.useMemo(() => {
    if (!availableShieldedAccountData) {
      return undefined
    }
    return accounts.find(
      (a) =>
        a.accountId.uniqueKey
        === availableShieldedAccountData.accountId.uniqueKey,
    )
  }, [accounts, availableShieldedAccountData])

  // Hooks
  const {
    selectedSellAsset,
    setSelectedSellAsset,
    sellAmount,
    setSellAmount,
    openSellAssetLink,
    checkIsAssetSellSupported,
  } = useMultiChainSellAssets()

  const { foundMeldBuyToken } = useFindBuySupportedToken(asset)
  const { openOrPushRoute } = useRoute()

  // Memos
  const isAssetsBalanceZero = React.useMemo(() => {
    return new Amount(assetBalance).isZero()
  }, [assetBalance])

  const canShieldFunds =
    availableShieldedAccountData && !isShieldedToken(asset) && !isAssetsBalanceZero

  const canUnshieldFunds =
    availableShieldedAccountData && isShieldedToken(asset) && !isAssetsBalanceZero

  const isSwapSupported = getDoesCoinSupportSwap(asset.coin)
  const isBridgeSupported = getDoesCoinSupportBridge(asset.coin)

  const isSellSupported = React.useMemo(() => {
    return account !== undefined && checkIsAssetSellSupported(asset)
  }, [account, checkIsAssetSellSupported, asset])

  // Methods
  const onClickBuy = React.useCallback(() => {
    if (foundMeldBuyToken) {
      openOrPushRoute(makeFundWalletRoute(foundMeldBuyToken, account))
    }
  }, [foundMeldBuyToken, openOrPushRoute, account])

  const onClickSend = React.useCallback(() => {
    openOrPushRoute(makeSendRoute(asset, account))
  }, [account, openOrPushRoute, asset])

  const onClickSwapOrBridge = React.useCallback(
    (routeType: 'swap' | 'bridge') => {
      openOrPushRoute(
        makeSwapOrBridgeRoute({
          fromToken: asset,
          fromAccount: account,
          routeType,
        }),
      )
    },
    [account, openOrPushRoute, asset],
  )

  const onClickDeposit = React.useCallback(() => {
    openOrPushRoute(makeDepositFundsRoute(getAssetIdKey(asset)))
  }, [asset, openOrPushRoute])

  const onClickSell = React.useCallback(() => {
    setSelectedSellAsset(asset)
    setShowSellModal(true)
  }, [setSelectedSellAsset, asset])

  const onOpenSellAssetLink = React.useCallback(() => {
    openSellAssetLink({
      sellAsset: selectedSellAsset,
    })
  }, [openSellAssetLink, selectedSellAsset])

  const onClickHide = React.useCallback(async () => {
    await updateUserAssetVisible({
      token: asset,
      isVisible: false,
    }).unwrap()
  }, [updateUserAssetVisible, asset])

  const onClickShieldFunds = React.useCallback(() => {
    if (!availableShieldedAccountData) {
      return
    }

    openOrPushRoute(
      makeSendRoute(
        asset,
        account,
        availableShieldedAccountData.zcashAccountInfo.orchardInternalAddress,
      ),
    )
  }, [availableShieldedAccountData, asset, openOrPushRoute, account])

  const onClickUnshieldFunds = React.useCallback(() => {
    if (
      !canUnshieldFunds
      || !shieldedAccount
      || !availableShieldedAccountData
    ) {
      return
    }
    openOrPushRoute(
      makeSendRoute(
        asset,
        shieldedAccount,
        availableShieldedAccountData.zcashAccountInfo
          .nextTransparentReceiveAddress.addressString,
      ),
    )
  }, [
    canUnshieldFunds,
    asset,
    openOrPushRoute,
    shieldedAccount,
    availableShieldedAccountData,
  ])

  return (
    <>
      <ButtonMenu placement='bottom-end'>
        <Button
          fab
          slot='anchor-content'
          kind='plain-faint'
          size='large'
        >
          <Icon name='more-vertical' />
        </Button>
        {foundMeldBuyToken && (
          <leo-menu-item onClick={onClickBuy}>
            <Icon name='coins-alt1' />
            {getLocale('braveWalletBuy')}
          </leo-menu-item>
        )}
        {!isAssetsBalanceZero && (
          <leo-menu-item onClick={onClickSend}>
            <Icon name='send' />
            {getLocale('braveWalletSend')}
          </leo-menu-item>
        )}
        {isSwapSupported && (
          <leo-menu-item onClick={() => onClickSwapOrBridge('swap')}>
            <Icon name='currency-exchange' />
            {getLocale('braveWalletSwap')}
          </leo-menu-item>
        )}
        {!isIOS && isBridgeSupported && (
          <leo-menu-item onClick={() => onClickSwapOrBridge('bridge')}>
            <Icon name='web3-bridge' />
            {getLocale('braveWalletBridge')}
          </leo-menu-item>
        )}
        <leo-menu-item onClick={onClickDeposit}>
          <Icon name='money-bag-coins' />
          {getLocale('braveWalletAccountsDeposit')}
        </leo-menu-item>
        {isSellSupported && (
          <leo-menu-item onClick={onClickSell}>
            <Icon name='usd-circle' />
            {getLocale('braveWalletSell')}
          </leo-menu-item>
        )}
        {onClickEditToken && (
          <leo-menu-item onClick={onClickEditToken}>
            <Icon name='edit-pencil' />
            {getLocale('braveWalletAllowSpendEditButton')}
          </leo-menu-item>
        )}
        <leo-menu-item onClick={onClickHide}>
          <Icon name='eye-off' />
          {getLocale('braveWalletConfirmHidingToken')}
        </leo-menu-item>
        {canShieldFunds && (
          <>
            <hr />
            <leo-menu-item onClick={onClickShieldFunds}>
              <Icon name='shield-done' />
              {getLocale('braveWalletShieldFunds')}
            </leo-menu-item>
          </>
        )}
        {canUnshieldFunds && (
          <>
            <hr />
            <leo-menu-item onClick={onClickUnshieldFunds}>
              <Icon name='shield-disable' />
              {getLocale('braveWalletUnshieldFunds')}
            </leo-menu-item>
          </>
        )}
      </ButtonMenu>
      {showSellModal && selectedSellAsset && (
        <SellAssetModal
          selectedAsset={selectedSellAsset}
          onClose={() => setShowSellModal(false)}
          sellAmount={sellAmount}
          setSellAmount={setSellAmount}
          openSellAssetLink={onOpenSellAssetLink}
          showSellModal={showSellModal}
          account={account}
          sellAssetBalance={assetBalance}
        />
      )}
    </>
  )
}
