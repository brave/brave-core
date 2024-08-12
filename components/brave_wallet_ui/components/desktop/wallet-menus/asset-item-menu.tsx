// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// Types
import { BraveWallet, WalletRoutes } from '../../../constants/types'

// Queries
import {
  useGetOnRampAssetsQuery,
  useUpdateUserAssetVisibleMutation //
} from '../../../common/slices/api.slice'

// Hooks
import {
  useMultiChainSellAssets //
} from '../../../common/hooks/use-multi-chain-sell-assets'

// Utils
import { getLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'
import {
  makeDepositFundsRoute,
  makeFundWalletRoute,
  makeSendRoute,
  makeSwapOrBridgeRoute
} from '../../../utils/routes-utils'
import { getAssetIdKey } from '../../../utils/asset-utils'

// Components
import {
  SellAssetModal //
} from '../popup-modals/sell-asset-modal/sell-asset-modal'

// Styled Components
import {
  PopupButton,
  MenuItemIcon,
  MoreVerticalIcon,
  ButtonMenu,
  MenuButton,
  MenuItemRow
} from './wallet_menus.style'

const coinSupportsSwap = (coin: BraveWallet.CoinType) => {
  return [BraveWallet.CoinType.ETH, BraveWallet.CoinType.SOL].includes(coin)
}

interface Props {
  asset: BraveWallet.BlockchainToken
  assetBalance: string
  account?: BraveWallet.AccountInfo
  onClickEditToken?: () => void
}

export const AssetItemMenu = (props: Props) => {
  const { asset, assetBalance, account, onClickEditToken } = props

  // routing
  const history = useHistory()

  // State
  const [showSellModal, setShowSellModal] = React.useState<boolean>(false)

  // Queries
  const { data: { allAssetOptions: allBuyAssetOptions } = {} } =
    useGetOnRampAssetsQuery()
  const [updateUserAssetVisible] = useUpdateUserAssetVisibleMutation()

  // Hooks
  const {
    selectedSellAsset,
    setSelectedSellAsset,
    sellAmount,
    setSellAmount,
    openSellAssetLink,
    checkIsAssetSellSupported
  } = useMultiChainSellAssets()

  // Memos
  const isAssetsBalanceZero = React.useMemo(() => {
    return new Amount(assetBalance).isZero()
  }, [assetBalance])

  const isBuySupported = React.useMemo(() => {
    if (!allBuyAssetOptions || isAssetsBalanceZero) {
      return false
    }
    return allBuyAssetOptions.some(
      (buyableAsset) =>
        buyableAsset.symbol.toLowerCase() === asset.symbol.toLowerCase()
    )
  }, [asset.symbol, allBuyAssetOptions, isAssetsBalanceZero])

  const isSwapSupported = coinSupportsSwap(asset.coin) && account !== undefined

  const isSellSupported = React.useMemo(() => {
    return account !== undefined && checkIsAssetSellSupported(asset)
  }, [account, checkIsAssetSellSupported, asset])

  // Methods
  const onClickBuy = React.useCallback(() => {
    history.push(makeFundWalletRoute(getAssetIdKey(asset)))
  }, [asset, history])

  const onClickSend = React.useCallback(() => {
    if (account) {
      history.push(makeSendRoute(asset, account))
    } else {
      history.push(WalletRoutes.Send)
    }
  }, [account, history, asset])

  const onClickSwap = React.useCallback(() => {
    if (account) {
      history.push(
        makeSwapOrBridgeRoute({
          fromToken: asset,
          fromAccount: account,
          routeType: 'swap'
        })
      )
    }
  }, [account, history, asset])

  const onClickDeposit = React.useCallback(() => {
    history.push(makeDepositFundsRoute(getAssetIdKey(asset)))
  }, [asset, history])

  const onClickSell = React.useCallback(() => {
    setSelectedSellAsset(asset)
    setShowSellModal(true)
  }, [setSelectedSellAsset, asset])

  const onOpenSellAssetLink = React.useCallback(() => {
    openSellAssetLink({
      sellAsset: selectedSellAsset
    })
  }, [openSellAssetLink, selectedSellAsset])

  const onClickHide = React.useCallback(async () => {
    await updateUserAssetVisible({
      token: asset,
      isVisible: false
    }).unwrap()
  }, [updateUserAssetVisible, asset])

  return (
    <ButtonMenu>
      <div slot='anchor-content'>
        <MenuButton
          kind='plain-faint'
          padding='0px'
        >
          <MoreVerticalIcon />
        </MenuButton>
      </div>

      {isBuySupported && (
        <PopupButton onClick={onClickBuy}>
          <MenuItemRow>
            <MenuItemIcon name='coins-alt1' />
            {getLocale('braveWalletBuy')}
          </MenuItemRow>
        </PopupButton>
      )}
      {!isAssetsBalanceZero && (
        <PopupButton onClick={onClickSend}>
          <MenuItemRow>
            <MenuItemIcon name='send' />
            {getLocale('braveWalletSend')}
          </MenuItemRow>
        </PopupButton>
      )}
      {isSwapSupported && !isAssetsBalanceZero && (
        <PopupButton onClick={onClickSwap}>
          <MenuItemRow>
            <MenuItemIcon name='currency-exchange' />
            {getLocale('braveWalletSwap')}
          </MenuItemRow>
        </PopupButton>
      )}
      <PopupButton onClick={onClickDeposit}>
        <MenuItemRow>
          <MenuItemIcon name='money-bag-coins' />
          {getLocale('braveWalletAccountsDeposit')}
        </MenuItemRow>
      </PopupButton>
      {isSellSupported && (
        <PopupButton onClick={onClickSell}>
          <MenuItemRow>
            <MenuItemIcon name='usd-circle' />
            {getLocale('braveWalletSell')}
          </MenuItemRow>
        </PopupButton>
      )}
      {onClickEditToken && (
        <PopupButton onClick={onClickEditToken}>
          <MenuItemRow>
            <MenuItemIcon name='edit-pencil' />
            {getLocale('braveWalletAllowSpendEditButton')}
          </MenuItemRow>
        </PopupButton>
      )}
      <PopupButton onClick={onClickHide}>
        <MenuItemRow>
          <MenuItemIcon name='eye-off' />
          {getLocale('braveWalletConfirmHidingToken')}
        </MenuItemRow>
      </PopupButton>

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
    </ButtonMenu>
  )
}
