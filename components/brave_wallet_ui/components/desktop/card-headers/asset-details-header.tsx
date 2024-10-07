// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Selectors
import { useSafeUISelector } from '../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../common/selectors'

// Utils
import { getLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'
import {
  getPriceIdForToken,
  getTokenPriceFromRegistry
} from '../../../utils/pricing-utils'
import { BraveWallet } from '../../../constants/types'
import {
  getIsRewardsToken,
  getRewardsTokenDescription
} from '../../../utils/rewards_utils'
import {
  externalWalletProviderFromString //
} from '../../../../brave_rewards/resources/shared/lib/external_wallet'
import { checkIfTokenNeedsNetworkIcon } from '../../../utils/asset-utils'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetNetworkQuery,
  useGetTokenSpotPricesQuery
} from '../../../common/slices/api.slice'
import { querySubscriptionOptions60s } from '../../../common/slices/constants'

// Hooks
import { useOnClickOutside } from '../../../common/hooks/useOnClickOutside'
import useExplorer from '../../../common/hooks/explorer'

// Components
import withPlaceholderIcon from '../../shared/create-placeholder-icon'
import { AssetDetailsMenu } from '../wallet-menus/asset-details-menu'
import { CreateNetworkIcon } from '../../shared/create-network-icon'

// Styled Components
import {
  MenuButton,
  MenuButtonIcon,
  MenuWrapper,
  HorizontalDivider
} from './shared-card-headers.style'
import {
  AssetIcon,
  AssetNameText,
  NetworkDescriptionText,
  PriceText,
  PercentChange,
  UpDownIcon,
  IconsWrapper,
  NetworkIconWrapper
} from './asset-details-header.style'
import { Button, ButtonIcon } from './shared-panel-headers.style'
import { Row, Column, HorizontalSpace } from '../../shared/style'
import { Skeleton } from '../../shared/loading-skeleton/styles'

const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, {
  size: 'big',
  marginLeft: 0,
  marginRight: 0
})

interface Props {
  selectedAsset?: BraveWallet.BlockchainToken
  onBack: () => void
  onClickTokenDetails: () => void
  onClickHideToken: () => void
  onClickEditToken?: () => void
  isShowingMarketData?: boolean
  selectedTimeline: BraveWallet.AssetPriceTimeframe
}

export const AssetDetailsHeader = (props: Props) => {
  const {
    selectedAsset,
    onBack,
    onClickHideToken,
    onClickTokenDetails,
    onClickEditToken,
    isShowingMarketData,
    selectedTimeline
  } = props

  // UI Selectors (safe)
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // queries
  const { data: selectedAssetsNetwork, isLoading: isLoadingNetwork } =
    useGetNetworkQuery(selectedAsset ?? skipToken)
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

  // state
  const [showAssetDetailsMenu, setShowAssetDetailsMenu] =
    React.useState<boolean>(false)

  // refs
  const assetDetailsMenuRef = React.useRef<HTMLDivElement>(null)

  // hooks
  useOnClickOutside(
    assetDetailsMenuRef,
    () => setShowAssetDetailsMenu(false),
    showAssetDetailsMenu
  )

  const openExplorer = useExplorer(selectedAssetsNetwork)

  // methods
  const handleOnClickHideToken = React.useCallback(() => {
    setShowAssetDetailsMenu(false)
    onClickHideToken()
  }, [onClickHideToken])

  const handleOnClickTokenDetails = React.useCallback(() => {
    setShowAssetDetailsMenu(false)
    onClickTokenDetails()
  }, [onClickTokenDetails])

  const handleOnClickEditToken = React.useCallback(() => {
    if (onClickEditToken) {
      setShowAssetDetailsMenu(false)
      onClickEditToken()
    }
  }, [onClickEditToken])

  const onClickViewOnExplorer = React.useCallback(() => {
    if (selectedAsset) {
      openExplorer('token', selectedAsset.contractAddress)()
    }
  }, [openExplorer, selectedAsset])

  const tokenPriceIds = React.useMemo(
    () => (selectedAsset ? [getPriceIdForToken(selectedAsset)] : []),
    [selectedAsset]
  )

  // queries
  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    tokenPriceIds.length && defaultFiatCurrency
      ? {
          ids: tokenPriceIds,
          timeframe: selectedTimeline,
          toCurrency: defaultFiatCurrency
        }
      : skipToken,
    querySubscriptionOptions60s
  )

  // computed
  const isRewardsToken = getIsRewardsToken(selectedAsset)

  const networkDescription = isShowingMarketData
    ? selectedAsset?.symbol ?? ''
    : isRewardsToken
    ? getRewardsTokenDescription(
        externalWalletProviderFromString(selectedAsset?.chainId ?? '')
      )
    : getLocale('braveWalletPortfolioAssetNetworkDescription')
        .replace('$1', selectedAsset?.symbol ?? '')
        .replace('$2', selectedAssetsNetwork?.chainName ?? '')

  const selectedAssetFiatPrice =
    selectedAsset &&
    spotPriceRegistry &&
    getTokenPriceFromRegistry(spotPriceRegistry, selectedAsset)

  const isSelectedAssetPriceDown = selectedAssetFiatPrice
    ? Number(selectedAssetFiatPrice.assetTimeframeChange) < 0
    : false

  return (
    <Row
      padding={isPanel ? '20px 16px' : '24px 0px'}
      justifyContent='space-between'
    >
      <Row width='unset'>
        {isPanel ? (
          <Row
            width='unset'
            margin='0px 12px 0px 0px'
          >
            <Button onClick={onBack}>
              <ButtonIcon name='carat-left' />
            </Button>
          </Row>
        ) : (
          <MenuButton
            marginRight={16}
            onClick={onBack}
          >
            <MenuButtonIcon
              size={16}
              name='arrow-left'
            />
          </MenuButton>
        )}
        <Row
          width='unset'
          gap='8px'
        >
          {selectedAsset ? (
            <IconsWrapper>
              <AssetIconWithPlaceholder asset={selectedAsset} />
              {selectedAssetsNetwork &&
                checkIfTokenNeedsNetworkIcon(
                  selectedAssetsNetwork,
                  selectedAsset.contractAddress
                ) && (
                  <NetworkIconWrapper>
                    <CreateNetworkIcon
                      network={selectedAssetsNetwork}
                      marginRight={0}
                    />
                  </NetworkIconWrapper>
                )}
            </IconsWrapper>
          ) : (
            <Skeleton
              height={'40px'}
              width={'40px'}
            />
          )}
          <Column alignItems='flex-start'>
            {selectedAsset ? (
              <AssetNameText>{selectedAsset?.name ?? ''}</AssetNameText>
            ) : (
              <Skeleton
                height={'18px'}
                width={'100px'}
              />
            )}
            {!selectedAsset || isLoadingNetwork ? (
              <Skeleton
                height={'16px'}
                width={'150px'}
              />
            ) : (
              <NetworkDescriptionText>
                {networkDescription}
              </NetworkDescriptionText>
            )}
          </Column>
        </Row>
      </Row>
      <Row width='unset'>
        <Column alignItems='flex-end'>
          <PriceText>
            {selectedAssetFiatPrice
              ? new Amount(selectedAssetFiatPrice.price).formatAsFiat(
                  defaultFiatCurrency
                )
              : '0.00'}
          </PriceText>

          {/* We may still keep BTC price value,
          keeping this around until decided. */}

          {/* <PriceText>
            {
              selectedAssetCryptoPrice
                ? new Amount(selectedAssetCryptoPrice.price)
                  .formatAsAsset(undefined, defaultCurrencies.crypto)
                : ''
            }
          </PriceText> */}

          <PercentChange isDown={isSelectedAssetPriceDown}>
            <UpDownIcon
              name={
                isSelectedAssetPriceDown ? 'arrow-small-down' : 'arrow-small-up'
              }
            />
            {selectedAssetFiatPrice
              ? Number(selectedAssetFiatPrice.assetTimeframeChange).toFixed(2)
              : '0.00'}
            %
          </PercentChange>
        </Column>
        {selectedAsset?.contractAddress &&
          !selectedAsset?.isErc721 &&
          !selectedAsset.isNft &&
          !isRewardsToken && (
            <>
              {isPanel ? (
                <HorizontalSpace space='12px' />
              ) : (
                <>
                  <HorizontalSpace space='16px' />
                  <HorizontalDivider />
                  <HorizontalSpace space='16px' />
                </>
              )}
              <MenuWrapper ref={assetDetailsMenuRef}>
                {isPanel ? (
                  <Button
                    onClick={() => setShowAssetDetailsMenu((prev) => !prev)}
                  >
                    <ButtonIcon name='more-vertical' />
                  </Button>
                ) : (
                  <MenuButton
                    onClick={() => setShowAssetDetailsMenu((prev) => !prev)}
                  >
                    <MenuButtonIcon name='more-vertical' />
                  </MenuButton>
                )}
                {showAssetDetailsMenu && (
                  <AssetDetailsMenu
                    assetSymbol={selectedAsset?.symbol ?? ''}
                    onClickHideToken={handleOnClickHideToken}
                    onClickTokenDetails={handleOnClickTokenDetails}
                    onClickViewOnExplorer={onClickViewOnExplorer}
                    onClickEditToken={handleOnClickEditToken}
                  />
                )}
              </MenuWrapper>
            </>
          )}
      </Row>
    </Row>
  )
}

export default AssetDetailsHeader
