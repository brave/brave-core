// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import {
  useSelector
} from 'react-redux'

// Proxies
import getWalletPanelApiProxy from '../../../panel/wallet_panel_api_proxy'

// Components
import { create, background } from 'ethereum-blockies'
import { CopyTooltip } from '../../shared/copy-tooltip/copy-tooltip'

// Utils
import { getLocale } from '../../../../common/locale'
import { reduceAddress } from '../../../utils/reduce-address'
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import Amount from '../../../utils/amount'
import { deserializeOrigin } from '../../../utils/model-serialization-utils'

// Hooks
import { useExplorer, usePricing } from '../../../common/hooks'
import { useGetSelectedChainQuery } from '../../../common/slices/api.slice'
import { useApiProxy } from '../../../common/hooks/use-api-proxy'

// types
import {
  PanelTypes,
  BraveWallet,
  WalletState,
  WalletOrigin
} from '../../../constants/types'

// Components
import {
  ConnectedHeader
} from '../'
import { SelectNetworkButton, LoadingSkeleton } from '../../shared'
import { PanelBottomNav } from '../panel-bottom-nav/panel-bottom-nav'

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
  navAction: (path: PanelTypes) => void
}

export const ConnectedPanel = (props: Props) => {
  const {
    navAction
  } = props

  const {
    defaultCurrencies,
    transactionSpotPrices: spotPrices,
    activeOrigin: originInfo,
    selectedAccount,
    connectedAccounts
  } = useSelector(({ wallet }: { wallet: WalletState }) => wallet)

  // queries
  const { currentData: selectedNetwork } = useGetSelectedChainQuery(undefined)
  const selectedCoin = selectedNetwork?.coin

  // state
  const [showMore, setShowMore] = React.useState<boolean>(false)
  const [isSolanaConnected, setIsSolanaConnected] = React.useState<boolean>(false)
  const [isPermissionDenied, setIsPermissionDenied] = React.useState<boolean>(false)

  // computed
  const selectedAccountAddress = selectedAccount?.address || ''
  const selectedAccountName = selectedAccount?.name || ''

  // custom hooks
  const { braveWalletService } = useApiProxy()
  const { computeFiatAmount } = usePricing(spotPrices)
  const onClickViewOnBlockExplorer = useExplorer(selectedNetwork)

  // methods
  const navigate = React.useCallback((path: PanelTypes) => () => {
    navAction(path)
  }, [navAction])

  const onExpand = React.useCallback(() => {
    navAction('expanded')
  }, [navAction])

  const onShowSitePermissions = React.useCallback(() => {
    if (isPermissionDenied) {
      const contentPath = selectedCoin === BraveWallet.CoinType.SOL ? 'solana' : 'ethereum'
      chrome.tabs.create({
        url: `brave://settings/content/${contentPath}`
      }).catch((e) => { console.error(e) })
      return
    }
    navAction('sitePermissions')
  }, [navAction, isPermissionDenied, selectedCoin])

  const onShowMore = React.useCallback(() => {
    setShowMore(true)
  }, [])

  const onHideMore = React.useCallback(() => {
    if (showMore) {
      setShowMore(false)
    }
  }, [showMore])

  // effects
  React.useEffect(() => {
    let subscribed = true

    if (selectedCoin) {
      (async () => {
        await braveWalletService.isPermissionDenied(selectedCoin, deserializeOrigin(originInfo.origin))
          .then(result => {
            if (subscribed) {
              setIsPermissionDenied(result.denied)
            }
          })
          .catch(e => console.log(e))
      })()
    }

    return () => {
      subscribed = false
    }
  }, [braveWalletService, selectedCoin, originInfo.origin])

  React.useEffect(() => {
    let subscribed = true

    if (selectedAccount?.address && selectedCoin === BraveWallet.CoinType.SOL) {
      (async () => {
        const { panelHandler } = getWalletPanelApiProxy()
        await panelHandler.isSolanaAccountConnected(selectedAccount?.address)
          .then(result => {
            if (subscribed) {
              setIsSolanaConnected(result.connected)
            }
          })
          .catch(e => console.log(e))
      })()
    }

    return () => {
      subscribed = false
    }
  }, [selectedAccount?.address, selectedCoin])

  // memos
  const bg = React.useMemo(() => {
    return background({ seed: selectedAccountAddress.toLowerCase() })
  }, [selectedAccountAddress])

  const orb = React.useMemo(() => {
    return create({ seed: selectedAccountAddress.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [selectedAccountAddress])

  const selectedAccountFiatBalance = React.useMemo(() => {
    if (!selectedNetwork || !selectedAccount) {
      return Amount.empty()
    }

    return computeFiatAmount(
      selectedAccount.nativeBalanceRegistry[selectedNetwork.chainId],
      selectedNetwork.symbol,
      selectedNetwork.decimals,
      '',
      selectedNetwork.chainId
    )
  }, [computeFiatAmount, selectedNetwork, selectedAccount])

  const isConnected = React.useMemo((): boolean => {
    if (selectedCoin === BraveWallet.CoinType.SOL) {
      return isSolanaConnected
    }
    if (originInfo.originSpec === WalletOrigin) {
      return true
    } else {
      return connectedAccounts.some(account => account.address === selectedAccountAddress)
    }
  }, [connectedAccounts, selectedAccountAddress, originInfo, selectedCoin, isSolanaConnected])

  const connectedStatusText = React.useMemo((): string => {
    if (isPermissionDenied) {
      return getLocale('braveWalletPanelBlocked')
    }
    if (selectedCoin === BraveWallet.CoinType.SOL) {
      return isConnected
        ? getLocale('braveWalletPanelConnected')
        : getLocale('braveWalletPanelDisconnected')
    }
    return isConnected
      ? getLocale('braveWalletPanelConnected')
      : getLocale('braveWalletPanelNotConnected')
  }, [isConnected, selectedCoin, isPermissionDenied])

  const showConnectButton = React.useMemo((): boolean => {
    if (isPermissionDenied) {
      return true
    }
    if (selectedCoin === BraveWallet.CoinType.SOL) {
      return connectedAccounts.length !== 0
    }
    return originInfo?.origin?.scheme !== 'chrome'
  }, [selectedCoin, connectedAccounts, originInfo, isPermissionDenied])

  // computed
  const formattedAssetBalance = selectedNetwork && selectedAccount
    ? new Amount(selectedAccount.nativeBalanceRegistry[selectedNetwork.chainId] ?? '')
      .divideByDecimals(selectedNetwork.decimals)
      .formatAsAsset(6, selectedNetwork.symbol)
    : ''

  // render
  return (
    <StyledWrapper onClick={onHideMore} panelBackground={bg}>
      <ConnectedHeader
        onExpand={onExpand}
        onClickMore={onShowMore}
        onClickViewOnBlockExplorer={selectedAccount ? onClickViewOnBlockExplorer('address', selectedAccountAddress) : undefined}
        showMore={showMore}
      />

      <CenterColumn>

        <StatusRow>
          <SelectNetworkButton
            onClick={navigate('networks')}
            selectedNetwork={selectedNetwork}
            isPanel={true}
          />
        </StatusRow>

        {showConnectButton ? (
          <StatusRow>
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
          </StatusRow>
        ) : (
          <div />
        )}

        <VerticalSpacer space='8px' />

        <BalanceColumn>
          <AccountCircle orb={orb} onClick={navigate('accounts')}>
            <SwitchIcon />
          </AccountCircle>
          <AccountNameText>{reduceAccountDisplayName(selectedAccountName, 24)}</AccountNameText>
          <CopyTooltip text={selectedAccountAddress}>
            <AccountAddressText>{reduceAddress(selectedAccountAddress)}</AccountAddressText>
          </CopyTooltip>
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
      <PanelBottomNav
        onNavigate={navAction}
      />
    </StyledWrapper>
  )
}

export default ConnectedPanel
