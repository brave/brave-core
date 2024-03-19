// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// Types
import { BraveWallet, WalletRoutes } from '../../../constants/types'

// Queries
import { useGetOnRampAssetsQuery } from '../../../common/slices/api.slice'

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
  makeSwapRoute
} from '../../../utils/routes-utils'
import { getAssetIdKey } from '../../../utils/asset-utils'

// Components
import {
  SellAssetModal //
} from '../popup-modals/sell-asset-modal/sell-asset-modal'

// Styled Components
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  ButtonIcon
} from './wellet-menus.style'

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

  // Hooks
  const {
    selectedSellAsset,
    setSelectedSellAsset,
    sellAmount,
    setSellAmount,
    selectedSellAssetNetwork,
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
      history.push(makeSwapRoute({ fromToken: asset, fromAccount: account }))
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

  return (
    <StyledWrapper yPosition={42}>
      {isBuySupported && (
        <PopupButton onClick={onClickBuy}>
          <ButtonIcon name='coins-alt1' />
          <PopupButtonText>{getLocale('braveWalletBuy')}</PopupButtonText>
        </PopupButton>
      )}
      {!isAssetsBalanceZero && (
        <PopupButton onClick={onClickSend}>
          <ButtonIcon name='send' />
          <PopupButtonText>{getLocale('braveWalletSend')}</PopupButtonText>
        </PopupButton>
      )}
      {isSwapSupported && !isAssetsBalanceZero && (
        <PopupButton onClick={onClickSwap}>
          <ButtonIcon name='currency-exchange' />
          <PopupButtonText>{getLocale('braveWalletSwap')}</PopupButtonText>
        </PopupButton>
      )}
      <PopupButton onClick={onClickDeposit}>
        <ButtonIcon name='money-bag-coins' />
        <PopupButtonText>
          {getLocale('braveWalletAccountsDeposit')}
        </PopupButtonText>
      </PopupButton>
      {isSellSupported && (
        <PopupButton onClick={onClickSell}>
          <ButtonIcon name='usd-circle' />
          <PopupButtonText>{getLocale('braveWalletSell')}</PopupButtonText>
        </PopupButton>
      )}
      {onClickEditToken && (
        <PopupButton onClick={onClickEditToken}>
          <ButtonIcon name='edit-pencil' />
          <PopupButtonText>
            {getLocale('braveWalletAllowSpendEditButton')}
          </PopupButtonText>
        </PopupButton>
      )}
      {showSellModal && selectedSellAsset && (
        <SellAssetModal
          selectedAsset={selectedSellAsset}
          selectedAssetsNetwork={selectedSellAssetNetwork}
          onClose={() => setShowSellModal(false)}
          sellAmount={sellAmount}
          setSellAmount={setSellAmount}
          openSellAssetLink={onOpenSellAssetLink}
          showSellModal={showSellModal}
          account={account}
          sellAssetBalance={assetBalance}
        />
      )}
    </StyledWrapper>
  )
}
