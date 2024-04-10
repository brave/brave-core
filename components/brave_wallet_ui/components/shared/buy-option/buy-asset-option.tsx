// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useParams } from 'react-router'
import { skipToken } from '@reduxjs/toolkit/query/react'

// types
import { BraveWallet } from '../../../constants/types'

// options
import {
  defaultQuerySubscriptionOptions //
} from '../../../common/slices/constants'

// utils
import Amount from '../../../utils/amount'
import { getLocale } from '../../../../common/locale'
import {
  checkIfTokenNeedsNetworkIcon,
  getAssetIdKey
} from '../../../utils/asset-utils'
import { getTokenPriceAmountFromRegistry } from '../../../utils/pricing-utils'
import { getPriceIdForToken } from '../../../utils/api-utils'

// hooks
import {
  useGetNetworkQuery,
  useGetTokenSpotPricesQuery
} from '../../../common/slices/api.slice'

// components
import {
  IconsWrapper,
  MediumAssetIcon,
  NetworkIconWrapper,
  Row
} from '../style'
import { withPlaceholderIcon } from '../create-placeholder-icon/index'
import { CreateNetworkIcon } from '../create-network-icon/index'
import { NftIcon } from '../nft-icon/nft-icon'

// styles
import {
  AssetButton,
  AssetName,
  NameAndIcon,
  NameColumn,
  NetworkDescriptionText,
  PriceContainer,
  PriceText
} from './buy-asset-option.styles'
import { LoadIcon } from './buy-option-item-styles'

interface Props {
  onClick?: (token: BraveWallet.BlockchainToken) => void
  token: BraveWallet.BlockchainToken
  isPanel?: boolean
  /** Set this to a currency-code to fetch & display the token's price */
  selectedCurrency?: string
}

const ICON_CONFIG = { size: 'big', marginLeft: 0, marginRight: 8 } as const
const AssetIconWithPlaceholder = withPlaceholderIcon(
  MediumAssetIcon,
  ICON_CONFIG
)
const NftAssetIconWithPlaceholder = withPlaceholderIcon(NftIcon, ICON_CONFIG)

export const BuyAssetOptionItem = React.forwardRef<HTMLDivElement, Props>(
  ({ onClick, token, isPanel, selectedCurrency }, ref) => {
    // routing
    const { assetId: selectedOnRampAssetId } = useParams<{ assetId: string }>()

    // query Params
    const tokenIds = React.useMemo(() => {
      return [getPriceIdForToken(token)]
    }, [token])

    // queries
    const {
      data: priceRegistry,
      isFetching: isFetchingPrice,
      isLoading: isLoadingPrice
    } = useGetTokenSpotPricesQuery(
      !tokenIds.length || !selectedCurrency
        ? skipToken
        : {
            ids: tokenIds,
            toCurrency: selectedCurrency
          },
      // refresh every 15 seconds
      defaultQuerySubscriptionOptions
    )
    const { data: tokenNetwork } = useGetNetworkQuery(token ?? skipToken)

    // memos
    const networkDescription: string = React.useMemo(() => {
      if (tokenNetwork && !isPanel) {
        return getLocale('braveWalletPortfolioAssetNetworkDescription')
          .replace('$1', token.symbol)
          .replace('$2', tokenNetwork.chainName ?? '')
      }
      return token.symbol
    }, [tokenNetwork, isPanel, token])

    const price = React.useMemo(() => {
      return priceRegistry
        ? getTokenPriceAmountFromRegistry(priceRegistry, token)
        : Amount.empty()
    }, [priceRegistry, token])

    // methods
    const handleOnClick = React.useCallback(() => {
      if (onClick) {
        onClick(token)
      }
    }, [onClick, token])

    // computed
    const isSelected = getAssetIdKey(token) === selectedOnRampAssetId

    // render
    if (!token.visible) {
      return null
    }

    return (
      <Row
        padding='6px 12px'
        ref={ref}
      >
        <AssetButton
          isSelected={isSelected}
          onClick={handleOnClick}
        >
          <NameAndIcon>
            <IconsWrapper marginRight='14px'>
              {token.isErc721 || token.isNft ? (
                <NftAssetIconWithPlaceholder
                  asset={token}
                  network={tokenNetwork}
                />
              ) : (
                <AssetIconWithPlaceholder
                  asset={token}
                  network={tokenNetwork}
                />
              )}
              {tokenNetwork &&
                !isPanel &&
                checkIfTokenNeedsNetworkIcon(
                  tokenNetwork,
                  token.contractAddress
                ) && (
                  <NetworkIconWrapper>
                    <CreateNetworkIcon
                      network={tokenNetwork}
                      marginRight={0}
                    />
                  </NetworkIconWrapper>
                )}
            </IconsWrapper>
            <NameColumn>
              <AssetName>
                {token.name}{' '}
                {token.isErc721 && token.tokenId
                  ? '#' + new Amount(token.tokenId).toNumber()
                  : ''}
              </AssetName>
              <NetworkDescriptionText>
                {networkDescription}
              </NetworkDescriptionText>
            </NameColumn>
          </NameAndIcon>

          {selectedCurrency && (
            <PriceContainer>
              {isFetchingPrice || isLoadingPrice ? (
                <LoadIcon />
              ) : (
                <PriceText>{price.formatAsFiat(selectedCurrency)}</PriceText>
              )}
            </PriceContainer>
          )}
        </AssetButton>
      </Row>
    )
  }
)

export default BuyAssetOptionItem
