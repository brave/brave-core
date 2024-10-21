// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import { BraveWallet } from '../../../../../constants/types'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetNetworkQuery,
  useGetTokenSpotPricesQuery
} from '../../../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s //
} from '../../../../../common/slices/constants'

// Hooks
import { useExplorer } from '../../../../../common/hooks/explorer'

// Utils
import Amount from '../../../../../utils/amount'
import { reduceAddress } from '../../../../../utils/reduce-address'
import { isNativeAsset } from '../../../../../utils/asset-utils'
import { getLocale } from '../../../../../../common/locale'
import {
  getPriceIdForToken,
  getTokenPriceFromRegistry
} from '../../../../../utils/pricing-utils'
import {
  getNFTTokenStandard,
  reduceInt
} from '../../../../../utils/string-utils'

// Components
import {
  withPlaceholderIcon //
} from '../../../../../components/shared/create-placeholder-icon/index'
import { NftIcon } from '../../../../../components/shared/nft-icon/nft-icon'
import {
  LoadingSkeleton //
} from '../../../../../components/shared/loading-skeleton'
import {
  CopyTooltip //
} from '../../../../../components/shared/copy-tooltip/copy-tooltip'

// Styles
import {
  Column,
  LeoSquaredButton,
  Row,
  Text
} from '../../../../../components/shared/style'
import { PercentChangeText, CopyIcon } from './token_details.style'
import { AssetIcon } from '../../shared_composer.style'

interface Props {
  token: BraveWallet.BlockchainToken
}

const ICON_CONFIG = { size: 'big', marginLeft: 0, marginRight: 8 } as const
const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, ICON_CONFIG)
const NftIconWithPlaceholder = withPlaceholderIcon(NftIcon, ICON_CONFIG)

export const TokenDetails = (props: Props) => {
  const { token } = props

  // Queries
  const { data: tokensNetwork } = useGetNetworkQuery(token)
  const { data: defaultFiatCurrency = 'usd' } = useGetDefaultFiatCurrencyQuery()
  const { data: spotPriceRegistry, isLoading: isLoadingSpotPrice } =
    useGetTokenSpotPricesQuery(
      defaultFiatCurrency
        ? { ids: [getPriceIdForToken(token)], toCurrency: defaultFiatCurrency }
        : skipToken,
      querySubscriptionOptions60s
    )

  // Hooks
  const onClickViewOnBlockExplorer = useExplorer(tokensNetwork)

  // Memos
  const formattedTokenId = React.useMemo(() => {
    return token.tokenId
      ? new Amount(token.tokenId).format(undefined, false)
      : ''
  }, [token.tokenId])

  // Computed
  const spotPrice = spotPriceRegistry
    ? getTokenPriceFromRegistry(spotPriceRegistry, token)
    : undefined

  const isNativeToken = isNativeAsset(token)

  const isNFT = token.isErc1155 || token.isErc721 || token.isNft

  const isPriceDown = spotPrice
    ? Number(spotPrice.assetTimeframeChange) < 0
    : false

  const tokenName = token.isShielded ? token.name + '(shielded)' : token.name

  return (
    <Column
      fullWidth={true}
      fullHeight={true}
      justifyContent='flex-start'
    >
      <Row
        margin='0px 0px 6px 0px'
        gap='8px'
      >
        {token.isErc721 || token.isNft ? (
          <NftIconWithPlaceholder asset={token} />
        ) : (
          <AssetIconWithPlaceholder asset={token} />
        )}
        <Text
          textSize='16px'
          textColor='primary'
          isBold={true}
        >
          {tokenName}
        </Text>
        <Text
          textSize='12px'
          textColor='secondary'
          isBold={false}
        >
          {token.symbol}
        </Text>
      </Row>
      <Column
        fullWidth={true}
        padding='16px 40px'
        gap='8px'
      >
        {!isNFT && (
          <Row justifyContent='space-between'>
            <Text
              textSize='14px'
              textColor='secondary'
              isBold={false}
            >
              {getLocale('braveWalletMarketPrice')}
            </Text>
            {isLoadingSpotPrice ? (
              <Column>
                <LoadingSkeleton
                  width={60}
                  height={14}
                />
              </Column>
            ) : (
              <Row
                width='unset'
                gap='2px'
              >
                <Text
                  textSize='14px'
                  textColor='primary'
                  isBold={true}
                >
                  {new Amount(spotPrice?.price ?? '').formatAsFiat(
                    defaultFiatCurrency
                  )}
                </Text>
                <PercentChangeText
                  isDown={isPriceDown}
                  textSize='14px'
                  isBold={false}
                >
                  {`${!isPriceDown ? '+' : ''}${Number(
                    spotPrice?.assetTimeframeChange ?? ''
                  ).toFixed(2)}%`}
                </PercentChangeText>
              </Row>
            )}
          </Row>
        )}
        <Row justifyContent='space-between'>
          <Text
            textSize='14px'
            textColor='secondary'
            isBold={false}
          >
            {getLocale('braveWalletAllowAddNetworkNetworkPanelTitle')}
          </Text>
          <Text
            textSize='14px'
            textColor='primary'
            isBold={true}
          >
            {tokensNetwork?.chainName ?? ''}
          </Text>
        </Row>
        {isNFT && (
          <Row justifyContent='space-between'>
            <Text
              textSize='14px'
              textColor='secondary'
              isBold={false}
            >
              {getLocale('braveWalletNFTDetailTokenID')}
            </Text>
            <CopyTooltip text={formattedTokenId}>
              <Row
                width='unset'
                gap='4px'
              >
                <Text
                  textSize='14px'
                  textColor='primary'
                  isBold={true}
                >
                  {'#' + reduceInt(formattedTokenId)}
                </Text>
                <CopyIcon />
              </Row>
            </CopyTooltip>
          </Row>
        )}
        {isNFT && (
          <Row justifyContent='space-between'>
            <Text
              textSize='14px'
              textColor='secondary'
              isBold={false}
            >
              {getLocale('braveWalletNFTDetailTokenStandard')}
            </Text>
            <Text
              textSize='14px'
              textColor='primary'
              isBold={true}
            >
              {getNFTTokenStandard(token)}
            </Text>
          </Row>
        )}
        {!isNativeToken && (
          <Row justifyContent='space-between'>
            <Text
              textSize='14px'
              textColor='secondary'
              isBold={false}
            >
              {getLocale('braveWalletContract')}
            </Text>
            <CopyTooltip text={token.contractAddress}>
              <Row
                width='unset'
                gap='4px'
              >
                <Text
                  textSize='14px'
                  textColor='primary'
                  isBold={true}
                >
                  {reduceAddress(token.contractAddress)}
                </Text>
                <CopyIcon />
              </Row>
            </CopyTooltip>
          </Row>
        )}
      </Column>
      {!isNativeToken && (
        <Row padding='16px'>
          <LeoSquaredButton
            onClick={
              isNFT
                ? onClickViewOnBlockExplorer(
                    'nft',
                    token.contractAddress,
                    token.tokenId
                  )
                : onClickViewOnBlockExplorer('token', token.contractAddress)
            }
            size='large'
          >
            {getLocale('braveWalletTransactionExplorer')}
          </LeoSquaredButton>
        </Row>
      )}
    </Column>
  )
}
