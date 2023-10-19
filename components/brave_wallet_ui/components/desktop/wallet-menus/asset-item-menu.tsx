// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'

// Types
import {
  BraveWallet,
  SendPageTabHashes,
  WalletRoutes
} from '../../../constants/types'

// Queries
import { useGetOnRampAssetsQuery } from '../../../common/slices/api.slice'

// Hooks
import { useMultiChainSellAssets } from '../../../common/hooks/use-multi-chain-sell-assets'

// Utils
import { getLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'
import { makeDepositFundsRoute } from '../../../utils/routes-utils'

// Components
import { SellAssetModal } from '../popup-modals/sell-asset-modal/sell-asset-modal'

// Styled Components
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  ButtonIcon
} from './wellet-menus.style'

interface Props {
  asset: BraveWallet.BlockchainToken
  assetBalance: string
  account?: BraveWallet.AccountInfo
}

export const AssetItemMenu = (props: Props) => {
  const { asset, assetBalance, account } = props

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

  const isSellSupported = React.useMemo(() => {
    return account !== undefined && checkIsAssetSellSupported(asset)
  }, [account, checkIsAssetSellSupported, asset])

  // Methods
  const onClickBuy = React.useCallback(() => {
    history.push(WalletRoutes.FundWalletPage.replace(':tokenId?', asset.symbol))
  }, [asset.symbol])

  const onClickSend = React.useCallback(() => {
    if (account) {
      const uniqueKeyOrAddress = account.address || account.accountId.uniqueKey

      const contractAddressOrSymbol =
        asset.contractAddress === '' ? asset.symbol : asset.contractAddress
      history.push(
        `${WalletRoutes.SendPage.replace(':chainId?', asset.chainId)
          .replace(':uniqueKeyOrAddress?', uniqueKeyOrAddress)
          .replace(':contractAddressOrSymbol?', contractAddressOrSymbol)
          .replace('/:tokenId?', '')}${
          //
          SendPageTabHashes.token
        }`
      )
    } else {
      history.push(WalletRoutes.SendPageStart)
    }
  }, [asset.chainId, asset.contractAddress, account?.address])

  const onClickSwap = React.useCallback(() => {
    history.push(WalletRoutes.Swap)
  }, [])

  const onClickDeposit = React.useCallback(() => {
    history.push(makeDepositFundsRoute(asset.symbol))
  }, [asset.symbol])

  const onClickSell = React.useCallback(() => {
    setSelectedSellAsset(asset)
    setShowSellModal(true)
  }, [setSelectedSellAsset, asset])

  const onOpenSellAssetLink = React.useCallback(() => {
    if (account?.address) {
      openSellAssetLink({
        sellAddress: account.address,
        sellAsset: selectedSellAsset
      })
    }
  }, [account?.address, openSellAssetLink])

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
      {!isAssetsBalanceZero && (
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
