// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Selectors
import {
  useUnsafePageSelector,
  useSafePageSelector,
  useUnsafeWalletSelector
} from '../../../common/hooks/use-safe-selector'
import { PageSelectors } from '../../../page/selectors'
import { WalletSelectors } from '../../../common/selectors'

// Utils
import { getLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'
import { getPriceIdForToken } from '../../../utils/api-utils'
import { getTokenPriceFromRegistry } from '../../../utils/pricing-utils'

// Queries
import {
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
  MenuWrapper
} from './shared-card-headers.style'
import {
  AssetIcon,
  AssetNameText,
  NetworkDescriptionText,
  PriceText,
  PercentChange,
  UpDownIcon,
  HorizontalDivider
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
  onBack: () => void
  onClickTokenDetails: () => void
  onClickHideToken: () => void
  isShowingMarketData?: boolean
}

export const AssetDetailsHeader = (props: Props) => {
  const {
    onBack,
    onClickHideToken,
    onClickTokenDetails,
    isShowingMarketData
  } = props

  // selectors
  const selectedAsset =
    useUnsafePageSelector(PageSelectors.selectedAsset)
  const selectedTimeline = useSafePageSelector(PageSelectors.selectedTimeline)
  const defaultCurrencies = useUnsafeWalletSelector(WalletSelectors.defaultCurrencies)

  // queries
  const { data: assetsNetwork } = useGetNetworkQuery(
    selectedAsset ?? skipToken
  )

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

  // queries
  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    selectedAsset
      ? {
        ids: [getPriceIdForToken(selectedAsset)],
        timeframe: selectedTimeline
      }
      : skipToken,
    querySubscriptionOptions60s
  )

  // computed
  const networkDescription =
    isShowingMarketData
      ? selectedAsset?.symbol ?? ''
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
      padding='24px 0px'
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
                  .formatAsFiat(defaultCurrencies.fiat)
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
