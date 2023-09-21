// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Utils
import { getLocale } from '../../../../common/locale'
import Amount from '../../../utils/amount'
import { openBlockExplorerURL } from '../../../utils/block-explorer-utils'

// Styled components
import {
  ExchangeRate,
  SwapDetails,
  SwapDetailsDivider,
  SwapAssetContainer,
  SwapAssetAddress,
  SwapAssetTitle,
  AddressOrb,
  AccountNameText,
  SwapAssetHeader,
  SwapDetailsArrow,
  SwapDetailsArrowContainer,
  SwapAssetDetailsContainer,
  AssetIcon,
  SwapAmountColumn,
  NetworkDescriptionText,
  Spacer,
  SwapAssetAmountSymbol,
  LaunchButton,
  LaunchIcon,
  SwapAmountRow
} from './swap.style'
import { CreateNetworkIcon } from '../../shared/create-network-icon/index'
import { LoadingSkeleton } from '../../shared/loading-skeleton/index'
import { withPlaceholderIcon } from '../../shared/create-placeholder-icon/index'
import {
  IconsWrapper as SwapAssetIconWrapper,
  NetworkIconWrapper
} from '../../shared/style'
import { NftIcon } from '../../shared/nft-icon/nft-icon'

// Types / constants
import { BraveWallet } from '../../../constants/types'
import {
  NATIVE_EVM_ASSET_CONTRACT_ADDRESS,
  UNKNOWN_TOKEN_COINGECKO_ID
} from '../../../common/constants/magics'

// Hooks
import {
  useGetNetworkQuery,
  useGetEthTokenDecimalsQuery,
  useGetEthTokenSymbolQuery
} from '../../../common/slices/api.slice'

type SwapToken = Pick<
  BraveWallet.BlockchainToken,
  | 'chainId'
  | 'coin'
  | 'symbol'
  | 'contractAddress'
  | 'logo'
  | 'isErc721'
  | 'isNft'
  | 'name'
  | 'coingeckoId'
  | 'decimals'
>

const isNativeToken = (token: SwapToken) =>
  token.contractAddress === '' ||
  token.contractAddress.toLowerCase() === NATIVE_EVM_ASSET_CONTRACT_ADDRESS

const makeToken = (
  token?: SwapToken,
  network?: BraveWallet.NetworkInfo,
  symbol?: string,
  decimals?: number
) => {
  if (!token || !network) {
    return undefined
  }

  if (token.coingeckoId !== UNKNOWN_TOKEN_COINGECKO_ID) {
    return token
  }

  if (isNativeToken(token)) {
    return {
      ...token,
      decimals: network.decimals,
      symbol: network.symbol
    }
  }

  if (!symbol || !decimals) {
    return undefined
  }

  return {
    ...token,
    symbol,
    decimals
  }
}


interface Props {
  sellToken?: SwapToken
  buyToken?: SwapToken
  sellAmount?: string
  buyAmount?: string
  senderOrb: string
  senderLabel?: string
  recipientOrb?: string
  recipientLabel?: string
  expectRecipientAddress?: boolean
}

export function SwapBase(props: Props) {
  const {
    sellToken,
    buyToken,
    sellAmount,
    buyAmount,
    senderLabel,
    senderOrb,
    recipientOrb,
    recipientLabel,
    expectRecipientAddress
  } = props

  // queries
  const { data: sellAssetNetwork } = useGetNetworkQuery(
    sellToken ?? skipToken
  )

  const { data: buyAssetNetwork } = useGetNetworkQuery(
    buyToken ?? skipToken
  )

  const {
    data: sellTokenDecimals
  } = useGetEthTokenDecimalsQuery(
    sellToken &&
    sellToken.coingeckoId === UNKNOWN_TOKEN_COINGECKO_ID &&
    !isNativeToken(sellToken)
      ? {
          chainId: sellToken.chainId,
          contractAddress: sellToken.contractAddress
       }
      : skipToken
  )
  const {
    data: sellTokenSymbol
  } = useGetEthTokenSymbolQuery(
    sellToken &&
    sellToken.coingeckoId === UNKNOWN_TOKEN_COINGECKO_ID &&
    !isNativeToken(sellToken)
      ? {
          chainId: sellToken.chainId,
          contractAddress: sellToken.contractAddress
        }
      : skipToken
  )

  const {
    data: buyTokenDecimals
  } = useGetEthTokenDecimalsQuery(
    buyToken &&
    buyToken.coingeckoId === UNKNOWN_TOKEN_COINGECKO_ID &&
    !isNativeToken(buyToken)
      ? {
          chainId: buyToken.chainId,
          contractAddress: buyToken.contractAddress
       }
      : skipToken
  )
  const {
    data: buyTokenSymbol
  } = useGetEthTokenSymbolQuery(
    buyToken &&
    buyToken.coingeckoId === UNKNOWN_TOKEN_COINGECKO_ID &&
    !isNativeToken(buyToken)
      ? {
          chainId: buyToken.chainId,
          contractAddress: buyToken.contractAddress
        }
      : skipToken
  )

  const buyTokenResult = React.useMemo(() =>
    makeToken(
      buyToken,
      buyAssetNetwork,
      buyTokenSymbol,
      buyTokenDecimals
    ),
    [
      buyToken,
      buyAssetNetwork,
      buyTokenSymbol,
      buyTokenDecimals
    ])
  const sellTokenResult = React.useMemo(() =>
    makeToken(
      sellToken,
      sellAssetNetwork,
      sellTokenSymbol,
      sellTokenDecimals
    ),
    [
      sellToken,
      sellAssetNetwork,
      sellTokenSymbol,
      sellTokenDecimals
    ])

  return (
    <>
      {buyTokenResult &&
        sellTokenResult &&
        buyAmount &&
        sellAmount && (
          <ExchangeRate>
            1 {sellTokenResult.symbol} ={' '}
            {new Amount(buyAmount)
              .divideByDecimals(buyTokenResult.decimals)
              .div(sellAmount)
              .multiplyByDecimals(sellTokenResult.decimals)
              .format(6)}{' '}
            {buyTokenResult.symbol}
          </ExchangeRate>
        )}
      <SwapDetails>
        <SwapAsset
          type='sell'
          network={sellAssetNetwork}
          address={senderLabel}
          orb={senderOrb}
          expectAddress={true}
          asset={sellTokenResult}
          amount={sellAmount}
        />

        <SwapDetailsDivider />
        <SwapDetailsArrowContainer>
          <SwapDetailsArrow />
        </SwapDetailsArrowContainer>

        <SwapAsset
          type='buy'
          network={buyAssetNetwork}
          address={recipientLabel}
          orb={recipientOrb}
          expectAddress={expectRecipientAddress}
          asset={buyTokenResult}
          amount={buyAmount}
        />
      </SwapDetails>
    </>
  )
}

interface SwapAssetProps {
  type: 'sell' | 'buy'
  network?: BraveWallet.NetworkInfo
  amount?: string
  asset?: SwapToken
  address?: string
  orb?: string
  expectAddress?: boolean
}

const ICON_CONFIG = { size: 'big', marginLeft: 0, marginRight: 8 } as const
const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, ICON_CONFIG)
const NftIconWithPlaceholder = withPlaceholderIcon(NftIcon, ICON_CONFIG)

function SwapAsset (props: SwapAssetProps) {
  const { type, address, orb, expectAddress, asset, amount, network } = props

  // computed
  const networkDescription = network
    ? getLocale('braveWalletPortfolioAssetNetworkDescription')
        .replace('$1', '')
        .replace('$2', network.chainName ?? '')
        .trim()
    : ''

  return (
    <SwapAssetContainer top={type === 'sell'}>
      <SwapAssetHeader>
        <SwapAssetTitle>
          {type === 'sell'
            ? getLocale('braveWalletSwapReviewSpend')
            : getLocale('braveWalletSwapReviewReceive')}
        </SwapAssetTitle>
        {expectAddress && (
          <SwapAssetAddress>
            {address && orb ? (
              <AddressOrb orb={orb} />
            ) : (
              <LoadingSkeleton width={10} height={10} circle={true} />
            )}

            {address ? (
              <AccountNameText>{address}</AccountNameText>
            ) : (
              <LoadingSkeleton width={84} />
            )}
          </SwapAssetAddress>
        )}
      </SwapAssetHeader>

      <SwapAssetDetailsContainer>
        <SwapAssetIconWrapper>
          {!AssetIconWithPlaceholder || !asset || !network ? (
            <LoadingSkeleton circle={true} width={40} height={40} />
          ) : (
            <>
              {asset.isErc721 ? (
                <NftIconWithPlaceholder asset={asset} network={network} />
              ) : (
                <AssetIconWithPlaceholder asset={asset} network={network} />
              )}
              {network && asset.contractAddress !== '' && (
                <NetworkIconWrapper>
                  <CreateNetworkIcon network={network} marginRight={0} />
                </NetworkIconWrapper>
              )}
            </>
          )}
        </SwapAssetIconWrapper>
        <SwapAmountColumn>
          {!networkDescription || !asset || !amount ? (
            <>
              <LoadingSkeleton width={200} height={18} />
              <Spacer />
              <LoadingSkeleton width={200} height={18} />
            </>
          ) : (
            <>
              <SwapAmountRow>
                <SwapAssetAmountSymbol>
                  {new Amount(amount)
                    .divideByDecimals(asset.decimals)
                    .formatAsAsset(6, asset.symbol)
                  }
                </SwapAssetAmountSymbol>
                  {asset.contractAddress !== '' &&
                    <LaunchButton
                      onClick={openBlockExplorerURL({
                        type: 'token',
                        network,
                        value: asset.contractAddress
                      })}
                    >
                      <LaunchIcon/>
                    </LaunchButton>
                  }
              </SwapAmountRow>
              <NetworkDescriptionText>
                {networkDescription}
              </NetworkDescriptionText>
            </>
          )}
        </SwapAmountColumn>
      </SwapAssetDetailsContainer>
    </SwapAssetContainer>
  )
}
