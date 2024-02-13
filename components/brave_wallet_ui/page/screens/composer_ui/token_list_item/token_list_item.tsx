// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import { BraveWallet } from '../../../../constants/types'

// Utils
import { getLocale } from '../../../../../common/locale'
import Amount from '../../../../utils/amount'
import { formatTokenBalanceWithSymbol } from '../../../../utils/balance-utils'
import { checkIfTokenNeedsNetworkIcon } from '../../../../utils/asset-utils'
import {
  useGetDefaultFiatCurrencyQuery,
  useGetNetworkQuery
} from '../../../../common/slices/api.slice'

// Components
import {
  withPlaceholderIcon //
} from '../../../../components/shared/create-placeholder-icon/index'
import {
  CreateNetworkIcon //
} from '../../../../components/shared/create-network-icon/index'
import { NftIcon } from '../../../../components/shared/nft-icon/nft-icon'
import {
  LoadingSkeleton //
} from '../../../../components/shared/loading-skeleton/index'
import { NFTInfoTooltip } from '../nft_info_tooltip/nft_info_tooltip'

// Styled Components
import {
  AssetIcon,
  NetworkIconWrapper,
  Button,
  IconsWrapper,
  ButtonWrapper,
  IconAndName,
  NameAndBalanceText,
  NetworkAndFiatText,
  DisabledLabel
} from './token_list_item.style'
import {
  Column,
  Row,
  HorizontalSpace
} from '../../../../components/shared/style'

interface Props {
  onClick: () => void
  token: BraveWallet.BlockchainToken
  balance?: string
  spotPrice?: string
  isLoadingSpotPrice?: boolean
  disabledText?: string
}

const ICON_CONFIG = { size: 'big', marginLeft: 0, marginRight: 0 } as const
const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, ICON_CONFIG)
const NftIconWithPlaceholder = withPlaceholderIcon(NftIcon, ICON_CONFIG)

export const TokenListItem = (props: Props) => {
  const {
    onClick,
    token,
    balance,
    spotPrice,
    isLoadingSpotPrice,
    disabledText
  } = props

  // Queries
  const { data: tokensNetwork } = useGetNetworkQuery(token ?? skipToken)
  const { data: defaultFiatCurrency = 'usd' } = useGetDefaultFiatCurrencyQuery()

  const fiatBalance = React.useMemo(() => {
    return balance && spotPrice
      ? new Amount(balance).divideByDecimals(token.decimals).times(spotPrice)
      : Amount.empty()
  }, [
    spotPrice,
    balance,
    token.symbol,
    token.decimals,
    token.contractAddress,
    token.chainId
  ])

  const formattedFiatBalance = fiatBalance.formatAsFiat(defaultFiatCurrency)

  const networkDescription = React.useMemo(() => {
    if (tokensNetwork) {
      return token.symbol !== ''
        ? getLocale('braveWalletPortfolioAssetNetworkDescription')
            .replace('$1', token.symbol)
            .replace('$2', tokensNetwork.chainName ?? '')
        : tokensNetwork.chainName
    }
    return token.symbol
  }, [tokensNetwork, token.symbol])

  const tokenDisplayName = React.useMemo(() => {
    if (token.isNft) {
      const id = token.tokenId ? `#${new Amount(token.tokenId).toNumber()}` : ''
      return `${token.name} ${id}`
    }
    if (disabledText) {
      return token.name.length > 12
        ? `${token.name.substring(0, 10)}...`
        : token.name
    }
    return token.name
  }, [token, disabledText])

  return (
    <ButtonWrapper width='100%'>
      <Button
        onClick={onClick}
        disabled={!!disabledText}
      >
        <IconAndName justifyContent='flex-start'>
          <IconsWrapper>
            {token.isErc721 || token.isNft ? (
              <NftIconWithPlaceholder
                asset={token}
                network={tokensNetwork}
              />
            ) : (
              <AssetIconWithPlaceholder
                asset={token}
                network={tokensNetwork}
              />
            )}
            {tokensNetwork &&
              checkIfTokenNeedsNetworkIcon(
                tokensNetwork,
                token.contractAddress
              ) && (
                <NetworkIconWrapper>
                  <CreateNetworkIcon
                    network={tokensNetwork}
                    marginRight={0}
                  />
                </NetworkIconWrapper>
              )}
          </IconsWrapper>
          <Column alignItems='flex-start'>
            <Row width='unset'>
              <NameAndBalanceText
                textSize='14px'
                isBold={true}
                textAlign='left'
              >
                {tokenDisplayName}
              </NameAndBalanceText>
              {disabledText && (
                <>
                  <HorizontalSpace space='8px' />
                  <DisabledLabel>{getLocale(disabledText)}</DisabledLabel>
                </>
              )}
            </Row>
            <NetworkAndFiatText
              textSize='12px'
              isBold={false}
              textAlign='left'
            >
              {networkDescription}
            </NetworkAndFiatText>
          </Column>
        </IconAndName>
        {balance && !token.isErc721 && !token.isNft && (
          <Column alignItems='flex-end'>
            <NameAndBalanceText
              textSize='14px'
              isBold={true}
              textAlign='right'
            >
              {formatTokenBalanceWithSymbol(
                balance,
                token.decimals,
                token.symbol
              )}
            </NameAndBalanceText>
            {isLoadingSpotPrice ? (
              <Column padding='3px 0px'>
                <LoadingSkeleton
                  width={40}
                  height={12}
                />
              </Column>
            ) : (
              <NetworkAndFiatText
                textSize='12px'
                isBold={false}
                textAlign='right'
              >
                {formattedFiatBalance}
              </NetworkAndFiatText>
            )}
          </Column>
        )}
      </Button>
      {(token.isErc721 || token.isNft) && (
        <NFTInfoTooltip
          network={tokensNetwork}
          token={token}
        />
      )}
    </ButtonWrapper>
  )
}

export default TokenListItem
