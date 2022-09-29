// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Options
import { BraveWallet, WalletState } from '../../../constants/types'

// Utils
import Amount from '../../../utils/amount'
import { getLocale } from '../../../../common/locale'
import { getTokensNetwork } from '../../../utils/network-utils'
import { computeFiatAmount } from '../../../utils/pricing-utils'
import { unbiasedRandom } from '../../../utils/random-utils'

// Components
import { withPlaceholderIcon, CreateNetworkIcon, LoadingSkeleton } from '../../shared'
import { WithHideBalancePlaceholder } from '../'
import { NftIcon } from '../../shared/nft-icon/nft-icon'

// Styled Components
import {
  StyledWrapper,
  AssetBalanceText,
  AssetName,
  BalanceColumn,
  FiatBalanceText,
  NameAndIcon,
  AssetIcon,
  NameColumn,
  Spacer,
  NetworkDescriptionText
} from './style'
import { IconsWrapper, NetworkIconWrapper } from '../../shared/style'
import { useSelector } from 'react-redux'

interface Props {
  action?: () => void
  assetBalance: string
  token: BraveWallet.BlockchainToken
  hideBalances?: boolean
  isPanel?: boolean
}

export const PortfolioAssetItem = ({
  assetBalance,
  action,
  token,
  hideBalances,
  isPanel
}: Props) => {
  // redux
  const defaultCurrencies = useSelector(({ wallet }: { wallet: WalletState }) => wallet.defaultCurrencies)
  const networks = useSelector(({ wallet }: { wallet: WalletState }) => wallet.networkList)
  const spotPrices = useSelector(({ wallet }: { wallet: WalletState }) => wallet.transactionSpotPrices)

  // state
  const [assetNameSkeletonWidth, setAssetNameSkeletonWidth] = React.useState(0)
  const [assetNetworkSkeletonWidth, setAssetNetworkSkeletonWidth] = React.useState(0)

  // memos & computed
  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(token.isErc721 ? NftIcon : AssetIcon, { size: 'big', marginLeft: 0, marginRight: 8 })
  }, [token.isErc721])

  const formattedAssetBalance = token.isErc721
    ? new Amount(assetBalance)
      .divideByDecimals(token.decimals)
      .format()
    : new Amount(assetBalance)
      .divideByDecimals(token.decimals)
      .formatAsAsset(6, token.symbol)

  const fiatBalance = React.useMemo(() => {
    return computeFiatAmount(spotPrices, { decimals: token.decimals, symbol: token.symbol, value: assetBalance })
  }, [spotPrices, assetBalance, token.symbol, token.decimals])

  const formattedFiatBalance = React.useMemo(() => {
    return fiatBalance.formatAsFiat(defaultCurrencies.fiat)
  }, [fiatBalance, defaultCurrencies.fiat])

  const isLoading = React.useMemo(() => {
    return formattedAssetBalance === '' && !token.isErc721
  }, [formattedAssetBalance, token])

  const tokensNetwork = React.useMemo(() => {
    return getTokensNetwork(networks, token)
  }, [token, networks])

  const NetworkDescription = React.useMemo(() => {
    if (tokensNetwork && !isPanel) {
      return getLocale('braveWalletPortfolioAssetNetworkDescription')
        .replace('$1', token.symbol)
        .replace('$2', tokensNetwork.chainName ?? '')
    }
    return token.symbol
  }, [tokensNetwork, token])

  // effects
  React.useEffect(() => {
    // Randow value between 100 & 250
    // Set value only once
    if (assetNameSkeletonWidth === 0) {
      setAssetNameSkeletonWidth(unbiasedRandom(100, 250))
    }

    if (assetNetworkSkeletonWidth === 0) {
      setAssetNetworkSkeletonWidth(unbiasedRandom(100, 250))
    }
  }, [])

  // render
  return (
    <>
      {token.visible &&
        // Selecting an erc721 token is temp disabled until UI is ready for viewing NFTs
        // or when showing loading skeleton
        <StyledWrapper disabled={isLoading} onClick={action}>
          <NameAndIcon>
            <IconsWrapper>
              {!token.logo
                ? <LoadingSkeleton
                  circle={true}
                  width={40}
                  height={40}
                />
                : <>
                  <AssetIconWithPlaceholder asset={token} network={tokensNetwork} />
                  {tokensNetwork && token.contractAddress !== '' && !isPanel &&
                    <NetworkIconWrapper>
                      <CreateNetworkIcon network={tokensNetwork} marginRight={0} />
                    </NetworkIconWrapper>
                  }
                </>
              }
            </IconsWrapper>
            <NameColumn>
              {!token.name && !token.symbol
                ? <>
                  <LoadingSkeleton width={assetNameSkeletonWidth} height={18} />
                  <Spacer />
                  <LoadingSkeleton width={assetNetworkSkeletonWidth} height={18} />
                </>
                : <>
                  <AssetName>
                    {token.name} {
                      token.isErc721 && token.tokenId
                        ? '#' + new Amount(token.tokenId).toNumber()
                        : ''
                    }
                  </AssetName>
                  <NetworkDescriptionText>{NetworkDescription}</NetworkDescriptionText>
                </>
              }
            </NameColumn>
          </NameAndIcon>
          <BalanceColumn>
            <WithHideBalancePlaceholder
              size='small'
              hideBalances={hideBalances ?? false}
            >

              {!token.isErc721 &&
                <>
                  {formattedFiatBalance ? (
                    <FiatBalanceText>{formattedFiatBalance}</FiatBalanceText>
                  ) : (
                    <>
                      <LoadingSkeleton width={60} height={18} />
                      <Spacer />
                    </>
                  )}
                </>
              }
              {formattedAssetBalance ? (
                <AssetBalanceText>{formattedAssetBalance}</AssetBalanceText>
              ) : (
                <LoadingSkeleton width={60} height={18} />
              )}
            </WithHideBalancePlaceholder>
          </BalanceColumn>
        </StyledWrapper>
      }
    </>
  )
}

export default PortfolioAssetItem
