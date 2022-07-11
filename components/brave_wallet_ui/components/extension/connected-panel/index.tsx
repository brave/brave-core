// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  useDispatch,
  useSelector
} from 'react-redux'

// Proxies
import getWalletPanelApiProxy from '../../../panel/wallet_panel_api_proxy'

// Actions
import { PanelActions } from '../../../panel/actions'

// Components
import { create, background } from 'ethereum-blockies'

// Utils
import { getLocale } from '../../../../common/locale'
import { reduceAddress } from '../../../utils/reduce-address'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import Amount from '../../../utils/amount'

// Hooks
import { useExplorer, usePricing, useIsMounted } from '../../../common/hooks'

// types
import {
  PanelTypes,
  BraveWallet,
  BuySupportedChains,
  WalletState,
  WalletOrigin
} from '../../../constants/types'

// Components
import {
  ConnectedBottomNav,
  ConnectedHeader
} from '../'
import { Tooltip, SelectNetworkButton, LoadingSkeleton } from '../../shared'

// Styled Components
import {
  StyledWrapper,
  AssetBalanceText,
  FiatBalanceText,
  AccountCircle,
  AccountAddressText,
  AccountNameText,
  CenterColumn,
  OvalButton,
  OvalButtonText,
  BigCheckMark,
  StatusRow,
  BalanceColumn,
  SwitchIcon,
  MoreAssetsButton,
  ConnectedStatusBubble
} from './style'

import { VerticalSpacer } from '../../shared/style'

export interface Props {
  isSwapSupported: boolean
  navAction: (path: PanelTypes) => void
}

export const ConnectedPanel = (props: Props) => {
  const {
    isSwapSupported,
    navAction
  } = props

  const dispatch = useDispatch()
  const {
    defaultCurrencies,
    transactionSpotPrices: spotPrices,
    activeOrigin: originInfo,
    selectedAccount,
    selectedNetwork,
    selectedCoin,
    connectedAccounts
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  // state
  const [showMore, setShowMore] = React.useState<boolean>(false)
  const [isSolanaConnected, setIsSolanaConnected] = React.useState<boolean>(false)

  // custom hooks
  const { computeFiatAmount } = usePricing(spotPrices)
  const onClickViewOnBlockExplorer = useExplorer(selectedNetwork)
  const isMounted = useIsMounted()

  // methods
  const navigate = React.useCallback((path: PanelTypes) => () => {
    navAction(path)
  }, [navAction])

  const onExpand = React.useCallback(() => {
    navAction('expanded')
  }, [navAction])

  const onShowSitePermissions = React.useCallback(() => {
    navAction('sitePermissions')
  }, [navAction])

  const onShowMore = React.useCallback(() => {
    setShowMore(true)
  }, [])

  const onHideMore = React.useCallback(() => {
    if (showMore) {
      setShowMore(false)
    }
  }, [showMore])

  const onCopyToClipboard = React.useCallback(async () => {
    await copyToClipboard(selectedAccount.address)
  }, [selectedAccount.address])

  const onOpenSettings = React.useCallback(() => {
    dispatch(PanelActions.openWalletSettings())
  }, [])

  // effects
  React.useEffect(() => {
    if (selectedCoin === BraveWallet.CoinType.SOL) {
      const isSolanaAccountConnected = async () => {
        const apiProxy = getWalletPanelApiProxy()
        await apiProxy.panelHandler.isSolanaAccountConnected(selectedAccount.address)
          .then(result => {
            if (isMounted) {
              setIsSolanaConnected(result.connected)
            }
          })
          .catch(e => console.log(e))
      }
      isSolanaAccountConnected()
    }
  }, [selectedAccount, selectedCoin, isMounted])

  // memos
  const bg = React.useMemo(() => {
    return background({ seed: selectedAccount.address.toLowerCase() })
  }, [selectedAccount.address])

  const orb = React.useMemo(() => {
    return create({ seed: selectedAccount.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [selectedAccount.address])

  const isBuyDisabled = React.useMemo(() => {
    return !BuySupportedChains.includes(selectedNetwork.chainId)
  }, [BuySupportedChains, selectedNetwork])

  const selectedAccountFiatBalance = React.useMemo(() => computeFiatAmount(
    selectedAccount.nativeBalanceRegistry[selectedNetwork.chainId], selectedNetwork.symbol, selectedNetwork.decimals
  ), [computeFiatAmount, selectedNetwork, selectedAccount])

  const isConnected = React.useMemo((): boolean => {
    if (selectedCoin === BraveWallet.CoinType.SOL) {
      return isSolanaConnected
    }
    if (originInfo.originSpec === WalletOrigin) {
      return true
    } else {
      return connectedAccounts.some(account => account.address === selectedAccount.address)
    }
  }, [connectedAccounts, selectedAccount, originInfo, selectedCoin, isSolanaConnected])

  const connectedStatusText = React.useMemo((): string => {
    if (selectedCoin === BraveWallet.CoinType.SOL) {
      return isConnected
        ? getLocale('braveWalletPanelConnected')
        : getLocale('braveWalletPanelDisconnected')
    }
    return isConnected
      ? getLocale('braveWalletPanelConnected')
      : getLocale('braveWalletPanelNotConnected')
  }, [isConnected, selectedCoin])

  const showConnectButton = React.useMemo((): boolean => {
    if (selectedCoin === BraveWallet.CoinType.SOL) {
      return connectedAccounts.length !== 0
    }
    return originInfo?.origin?.scheme !== 'chrome'
  }, [selectedCoin, connectedAccounts, originInfo])

  // computed
  const formattedAssetBalance = new Amount(selectedAccount.nativeBalanceRegistry[selectedNetwork.chainId] ?? '')
    .divideByDecimals(selectedNetwork.decimals)
    .formatAsAsset(6, selectedNetwork.symbol)

  // render
  return (
    <StyledWrapper onClick={onHideMore} panelBackground={bg}>
      <ConnectedHeader
        onExpand={onExpand}
        onClickSetting={onOpenSettings}
        onClickMore={onShowMore}
        onClickViewOnBlockExplorer={onClickViewOnBlockExplorer('address', selectedAccount.address)}
        showMore={showMore}
      />
      <CenterColumn>
        <StatusRow>
          {showConnectButton ? (
            <OvalButton onClick={onShowSitePermissions}>
              {selectedCoin === BraveWallet.CoinType.SOL ? (
                <ConnectedStatusBubble isConnected={isConnected} />
              ) : (
                <>
                  {isConnected && <BigCheckMark />}
                </>
              )}
              <OvalButtonText>{connectedStatusText}</OvalButtonText>
            </OvalButton>
          ) : (
            <div />
          )}
          <Tooltip
            text={selectedNetwork.chainName}
            position='right'
          >
            <SelectNetworkButton
              onClick={navigate('networks')}
              selectedNetwork={selectedNetwork}
              isPanel={true}
            />
          </Tooltip>
        </StatusRow>
        <BalanceColumn>
          <AccountCircle orb={orb} onClick={navigate('accounts')}>
            <SwitchIcon />
          </AccountCircle>
          <AccountNameText>{reduceAccountDisplayName(selectedAccount.name, 14)}</AccountNameText>
          <Tooltip text={getLocale('braveWalletToolTipCopyToClipboard')}>
            <AccountAddressText onClick={onCopyToClipboard}>{reduceAddress(selectedAccount.address)}</AccountAddressText>
          </Tooltip>
        </BalanceColumn>
        <BalanceColumn>
          {formattedAssetBalance ? (
            <AssetBalanceText>{formattedAssetBalance}</AssetBalanceText>
          ) : (
            <>
              <VerticalSpacer space={6} />
              <LoadingSkeleton useLightTheme={true} width={120} height={24} />
              <VerticalSpacer space={6} />
            </>
          )}
          {!selectedAccountFiatBalance.isUndefined() ? (
            <FiatBalanceText>
              {selectedAccountFiatBalance.formatAsFiat(defaultCurrencies.fiat)}
            </FiatBalanceText>
          ) : (
            <LoadingSkeleton useLightTheme={true} width={80} height={20} />
          )}
        </BalanceColumn>
        <MoreAssetsButton onClick={navigate('assets')}>{getLocale('braveWalletPanelViewAccountAssets')}</MoreAssetsButton>
      </CenterColumn>
      <ConnectedBottomNav
        selectedNetwork={selectedNetwork}
        isBuyDisabled={isBuyDisabled}
        isSwapDisabled={!isSwapSupported}
        onNavigate={navAction}
      />
    </StyledWrapper>
  )
}

export default ConnectedPanel
