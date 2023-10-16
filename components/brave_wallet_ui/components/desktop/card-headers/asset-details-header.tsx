// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Selectors
import {
  useSafePageSelector,
  useSafeUISelector
} from '../../../common/hooks/use-safe-selector'
import { PageSelectors } from '../../../page/selectors'
import {
  UISelectors
} from '../../../common/selectors'

// Utils
import { getLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'
import { getPriceIdForToken } from '../../../utils/api-utils'
import { getTokenPriceFromRegistry } from '../../../utils/pricing-utils'
import { BraveWallet } from '../../../constants/types'
import {
  getIsRewardsToken, getRewardsTokenDescription
} from '../../../utils/rewards_utils'
import {
  externalWalletProviderFromString
} from '../../../../brave_rewards/resources/shared/lib/external_wallet'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetNetworkQuery,
  useGetSelectedChainQuery,
  useGetTokenSpotPricesQuery
} from '../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s
} from '../../../common/slices/constants'

// Hooks
import {
  useOnClickOutside
} from '../../../common/hooks/useOnClickOutside'
import useExplorer from '../../../common/hooks/explorer'

// Components
import withPlaceholderIcon from '../../shared/create-placeholder-icon'
import { AssetDetailsMenu } from '../wallet-menus/asset-details-menu'

// Styled Components
import {
  CircleButton,
  ButtonIcon,
  MenuWrapper,
  HorizontalDivider
} from './shared-card-headers.style'
import {
  AssetIcon,
  AssetNameText,
  NetworkDescriptionText,
  PriceText,
  PercentChange,
  UpDownIcon
} from './asset-details-header.style'
import { Row, Column, HorizontalSpace } from '../../shared/style'

const AssetIconWithPlaceholder =
  withPlaceholderIcon(
    AssetIcon,
    {
      size: 'big',
      marginLeft: 0,
      marginRight: 8
    }
  )

interface Props {
  selectedAsset?: BraveWallet.BlockchainToken
  onBack: () => void
  onClickTokenDetails: () => void
  onClickHideToken: () => void
  isShowingMarketData?: boolean
}

export const AssetDetailsHeader = (props: Props) => {
  const {
    selectedAsset,
    onBack,
    onClickHideToken,
    onClickTokenDetails,
    isShowingMarketData
  } = props

  // selectors
  const selectedTimeline = useSafePageSelector(PageSelectors.selectedTimeline)

  // UI Selectors (safe)
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // queries
  const { data: assetsNetwork } = useGetNetworkQuery(
    selectedAsset ?? skipToken
  )
  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

  const { data: selectedNetwork } = useGetSelectedChainQuery(undefined, {
    skip: !!assetsNetwork
  })
  const selectedAssetsNetwork = assetsNetwork || selectedNetwork

  // state
  const [showAssetDetailsMenu, setShowAssetDetailsMenu] =
    React.useState<boolean>(false)

  // refs
  const assetDetailsMenuRef =
    React.useRef<HTMLDivElement>(null)

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

  const onClickViewOnExplorer = React.useCallback(() => {
    if (selectedAsset) {
      openExplorer('token', selectedAsset.contractAddress)()
    }
  }, [selectedAsset])

  const tokenPriceIds = React.useMemo(() =>
    selectedAsset
      ? [getPriceIdForToken(selectedAsset)]
      : [],
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

  const networkDescription =
    isShowingMarketData
      ? selectedAsset?.symbol ?? ''
      : isRewardsToken
        ? getRewardsTokenDescription(
          externalWalletProviderFromString(selectedAsset?.chainId ?? '')
        )
        : getLocale('braveWalletPortfolioAssetNetworkDescription')
          .replace('$1', selectedAsset?.symbol ?? '')
          .replace('$2', selectedAssetsNetwork?.chainName ?? '')

  const selectedAssetFiatPrice = selectedAsset &&
    spotPriceRegistry &&
    getTokenPriceFromRegistry(spotPriceRegistry, selectedAsset)

  const isSelectedAssetPriceDown =
    selectedAssetFiatPrice
      ? Number(selectedAssetFiatPrice.assetTimeframeChange) < 0
      : false

  return (
    <Row
      padding={isPanel ? '12px 20px' : '24px 0px'}
      justifyContent='space-between'
    >
      <Row
        width='unset'
      >
        <CircleButton
          size={28}
          marginRight={16}
          onClick={onBack}
        >
          <ButtonIcon
            size={16}
            name='arrow-left' />
        </CircleButton>
        <Row
          width='unset'
        >
          <AssetIconWithPlaceholder
            asset={selectedAsset}
            network={selectedAssetsNetwork}
          />
          <Column
            alignItems='flex-start'
          >
            <AssetNameText>
              {selectedAsset?.name ?? ''}
            </AssetNameText>
            <NetworkDescriptionText>
              {networkDescription}
            </NetworkDescriptionText>
          </Column>
        </Row>
      </Row>
      <Row
        width='unset'
      >
        <Column
          alignItems='flex-end'
        >
          <PriceText>
            {
              selectedAssetFiatPrice
                ? new Amount(selectedAssetFiatPrice.price)
                  .formatAsFiat(defaultFiatCurrency)
                : '0.00'
            }
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

          <PercentChange
            isDown={isSelectedAssetPriceDown}
          >
            <UpDownIcon
              name={
                isSelectedAssetPriceDown
                  ? 'arrow-small-down'
                  : 'arrow-small-up'
              }
            />
            {
              selectedAssetFiatPrice
                ? Number(selectedAssetFiatPrice.assetTimeframeChange)
                  .toFixed(2)
                : '0.00'
            }%
          </PercentChange>
        </Column>
        {selectedAsset?.contractAddress &&
          !selectedAsset?.isErc721 &&
          !selectedAsset.isNft &&
          !isRewardsToken &&
          <>
            <HorizontalSpace space='16px' />
            <HorizontalDivider />
            <HorizontalSpace space='16px' />
            <MenuWrapper
              ref={assetDetailsMenuRef}
            >
              <CircleButton
                onClick={
                  () => setShowAssetDetailsMenu(prev => !prev)
                }
              >
                <ButtonIcon
                  name='more-vertical' />
              </CircleButton>
              {showAssetDetailsMenu &&
                <AssetDetailsMenu
                  assetSymbol={selectedAsset?.symbol ?? ''}
                  onClickHideToken={handleOnClickHideToken}
                  onClickTokenDetails={handleOnClickTokenDetails}
                  onClickViewOnExplorer={onClickViewOnExplorer}
                />
              }
            </MenuWrapper>
          </>
        }
      </Row>
    </Row>
  )
}

export default AssetDetailsHeader
