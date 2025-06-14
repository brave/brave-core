// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useHistory } from 'react-router'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import { BraveWallet } from '../../../constants/types'

// Selectors
import {
  useSafeWalletSelector, //
} from '../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../common/selectors'

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
  getDoesCoinSupportSwapOrBridge,
} from '../../../utils/asset-utils'

// Components
import {
  SellAssetModal, //
} from '../popup-modals/sell-asset-modal/sell-asset-modal'

// Styled Components
import {
  StyledWrapper,
  PopupButton,
  PopupButtonText,
  ButtonIcon,
} from './wellet-menus.style'
import { VerticalDivider } from '../../shared/style'

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

  // Selectors
  const isZCashShieldedTransactionsEnabled = useSafeWalletSelector(
    WalletSelectors.isZCashShieldedTransactionsEnabled,
  )

  // Mutations
  const [updateUserAssetVisible] = useUpdateUserAssetVisibleMutation()

  // Queries
  const { accounts } = useAccountsQuery()
  const zcashAccountIds = accounts
    .filter((account) => account.accountId.coin === BraveWallet.CoinType.ZEC)
    .map((account) => account.accountId)

  const { data: availableShieldedAccount } =
    useGetAvailableShieldedAccountQuery(
      asset.coin === BraveWallet.CoinType.ZEC
        && !asset.isShielded
        && isZCashShieldedTransactionsEnabled
        && zcashAccountIds
        ? zcashAccountIds
        : skipToken,
    )

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

  // Memos
  const isAssetsBalanceZero = React.useMemo(() => {
    return new Amount(assetBalance).isZero()
  }, [assetBalance])

  const isSwapOrBridgeSupported = getDoesCoinSupportSwapOrBridge(asset.coin)

  const isSellSupported = React.useMemo(() => {
    return account !== undefined && checkIsAssetSellSupported(asset)
  }, [account, checkIsAssetSellSupported, asset])

  // Methods
  const onClickBuy = React.useCallback(() => {
    if (foundMeldBuyToken) {
      history.push(makeFundWalletRoute(foundMeldBuyToken, account))
    }
  }, [foundMeldBuyToken, history, account])

  const onClickSend = React.useCallback(() => {
    history.push(makeSendRoute(asset, account))
  }, [account, history, asset])

  const onClickSwapOrBridge = React.useCallback(
    (routeType: 'swap' | 'bridge') => {
      history.push(
        makeSwapOrBridgeRoute({
          fromToken: asset,
          fromAccount: account,
          routeType,
        }),
      )
    },
    [account, history, asset],
  )

  const onClickDeposit = React.useCallback(() => {
    history.push(makeDepositFundsRoute(getAssetIdKey(asset)))
  }, [asset, history])

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
    if (!availableShieldedAccount) {
      return
    }

    history.push(
      makeSendRoute(
        asset,
        account,
        availableShieldedAccount.orchardInternalAddress,
      ),
    )
  }, [availableShieldedAccount, asset, history, account])

  return (
    <StyledWrapper yPosition={42}>
      {foundMeldBuyToken && (
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
      {isSwapOrBridgeSupported && (
        <>
          <PopupButton onClick={() => onClickSwapOrBridge('swap')}>
            <ButtonIcon name='currency-exchange' />
            <PopupButtonText>{getLocale('braveWalletSwap')}</PopupButtonText>
          </PopupButton>
          <PopupButton onClick={() => onClickSwapOrBridge('bridge')}>
            <ButtonIcon name='web3-bridge' />
            <PopupButtonText>{getLocale('braveWalletBridge')}</PopupButtonText>
          </PopupButton>
        </>
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
      <PopupButton onClick={onClickHide}>
        <ButtonIcon name='eye-off' />
        <PopupButtonText>
          {getLocale('braveWalletConfirmHidingToken')}
        </PopupButtonText>
      </PopupButton>
      {availableShieldedAccount && (
        <>
          <VerticalDivider margin='0px 0px 8px 0px' />
          <PopupButton onClick={onClickShieldFunds}>
            <ButtonIcon name='shield-done' />
            <PopupButtonText>
              {getLocale('braveWalletShieldFunds')}
            </PopupButtonText>
          </PopupButton>
        </>
      )}
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
    </StyledWrapper>
  )
}
