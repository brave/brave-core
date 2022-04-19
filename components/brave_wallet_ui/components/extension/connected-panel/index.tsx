// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { create, background } from 'ethereum-blockies'

// Utils
import { getLocale } from '../../../../common/locale'
import { reduceAddress } from '../../../utils/reduce-address'
import { copyToClipboard } from '../../../utils/copy-to-clipboard'
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import Amount from '../../../utils/amount'

// Hooks
import { useExplorer, usePricing } from '../../../common/hooks'

// types
import {
  WalletAccountType,
  PanelTypes,
  BraveWallet,
  BuySupportedChains,
  DefaultCurrencies
} from '../../../constants/types'

// Components
import {
  ConnectedBottomNav,
  ConnectedHeader
} from '../'
import { Tooltip, SelectNetworkButton } from '../../shared'

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
  MoreAssetsButton
} from './style'

export interface Props {
  spotPrices: BraveWallet.AssetPrice[]
  selectedAccount: WalletAccountType
  selectedNetwork: BraveWallet.NetworkInfo
  isConnected: boolean
  originInfo: BraveWallet.OriginInfo
  isSwapSupported: boolean
  defaultCurrencies: DefaultCurrencies
  navAction: (path: PanelTypes) => void
  onOpenSettings: () => void
}

export const ConnectedPanel = (props: Props) => {
  const {
    spotPrices,
    onOpenSettings,
    isConnected,
    isSwapSupported,
    navAction,
    selectedAccount,
    selectedNetwork,
    originInfo,
    defaultCurrencies
  } = props

  // state
  const [showMore, setShowMore] = React.useState<boolean>(false)

  // custom hooks
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
          {originInfo?.origin?.scheme !== 'chrome' ? (
            <OvalButton onClick={onShowSitePermissions}>
              {isConnected && <BigCheckMark />}
              <OvalButtonText>{isConnected ? getLocale('braveWalletPanelConnected') : getLocale('braveWalletPanelNotConnected')}</OvalButtonText>
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
          <AssetBalanceText>{formattedAssetBalance}</AssetBalanceText>
          <FiatBalanceText>
            {selectedAccountFiatBalance.formatAsFiat(defaultCurrencies.fiat)}
          </FiatBalanceText>
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
