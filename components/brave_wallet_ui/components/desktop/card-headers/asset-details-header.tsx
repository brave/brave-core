// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Selectors
import {
  useUnsafePageSelector,
  useUnsafeWalletSelector
} from '../../../common/hooks/use-safe-selector'
import { PageSelectors } from '../../../page/selectors'
import { WalletSelectors } from '../../../common/selectors'

// Utils
import { getLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'

// Queries
import {
  useGetNetworkQuery,
  useGetSelectedChainQuery
} from '../../../common/slices/api.slice'

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
  const selectedAssetFiatPrice =
    useUnsafePageSelector(PageSelectors.selectedAssetFiatPrice)

  // Keeping this around for BTC value incase we need it.
  // const selectedAssetCryptoPrice =
  //   useUnsafePageSelector(PageSelectors.selectedAssetCryptoPrice)

  const defaultCurrencies =
    useUnsafeWalletSelector(WalletSelectors.defaultCurrencies)

  // queries
  const { data: assetsNetwork } = useGetNetworkQuery(selectedAsset, {
    skip: !selectedAsset
  })

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

  // computed
  const networkDescription =
    isShowingMarketData
      ? selectedAsset?.symbol ?? ''
      : getLocale('braveWalletPortfolioAssetNetworkDescription')
        .replace('$1', selectedAsset?.symbol ?? '')
        .replace('$2', selectedAssetsNetwork?.chainName ?? '')

  const isSelectedAssetPriceDown =
    selectedAsset &&
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
