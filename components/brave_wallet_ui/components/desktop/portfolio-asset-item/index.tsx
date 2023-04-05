// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'

// Options
import { BraveWallet, WalletState } from '../../../constants/types'

// Utils
import Amount from '../../../utils/amount'
import { getLocale } from '../../../../common/locale'
import { computeFiatAmount } from '../../../utils/pricing-utils'
import { unbiasedRandom } from '../../../utils/random-utils'
import { isDataURL } from '../../../utils/string-utils'
import { checkIfTokenNeedsNetworkIcon } from '../../../utils/asset-utils'

// Hooks
import { useGetNetworkQuery } from '../../../common/slices/api.slice'

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
  NetworkDescriptionText,
  ButtonArea
} from './style'
import { IconsWrapper, NetworkIconWrapper, SellButton, SellButtonRow } from '../../shared/style'

interface Props {
  action?: () => void
  assetBalance: string
  token: BraveWallet.BlockchainToken
  hideBalances?: boolean
  isPanel?: boolean
  isAccountDetails?: boolean
  isSellSupported?: boolean
  showSellModal?: () => void
}

export const PortfolioAssetItem = ({
  assetBalance,
  action,
  token,
  hideBalances,
  isPanel,
  isAccountDetails,
  isSellSupported,
  showSellModal
}: Props) => {
  // redux
  const defaultCurrencies = useSelector(({ wallet }: { wallet: WalletState }) => wallet.defaultCurrencies)
  const spotPrices = useSelector(({ wallet }: { wallet: WalletState }) => wallet.transactionSpotPrices)

  // queries
  const { data: tokensNetwork } = useGetNetworkQuery(token, { skip: !token })

  // state
  const [assetNameSkeletonWidth, setAssetNameSkeletonWidth] = React.useState(0)
  const [assetNetworkSkeletonWidth, setAssetNetworkSkeletonWidth] = React.useState(0)

  // memos & computed
  const isNonFungibleToken = React.useMemo(() => token.isNft || token.isErc721, [token.isNft, token.isErc721])

  const AssetIconWithPlaceholder = React.useMemo(() => {
    return withPlaceholderIcon(isNonFungibleToken && !isDataURL(token.logo) ? NftIcon : AssetIcon, { size: 'big', marginLeft: 0, marginRight: 8 })
  }, [isNonFungibleToken, token.logo])

  const formattedAssetBalance = isNonFungibleToken
    ? new Amount(assetBalance)
      .divideByDecimals(token.decimals)
      .format()
    : new Amount(assetBalance)
      .divideByDecimals(token.decimals)
      .formatAsAsset(6, token.symbol)

  const fiatBalance = React.useMemo(() => {
    return computeFiatAmount(spotPrices, {
      decimals: token.decimals,
      symbol: token.symbol,
      value: assetBalance,
      contractAddress: token.contractAddress,
      chainId: token.chainId
    })
  }, [spotPrices, assetBalance, token.symbol, token.decimals, token.chainId])

  const formattedFiatBalance = React.useMemo(() => {
    return fiatBalance.formatAsFiat(defaultCurrencies.fiat)
  }, [fiatBalance, defaultCurrencies.fiat])

  const isLoading = React.useMemo(() => {
    return formattedAssetBalance === '' && !isNonFungibleToken
  }, [formattedAssetBalance, token])

  const NetworkDescription = React.useMemo(() => {

    if (tokensNetwork && !isPanel) {
      return token.symbol !== ''
      ? getLocale('braveWalletPortfolioAssetNetworkDescription')
        .replace('$1', token.symbol)
        .replace('$2', tokensNetwork.chainName ?? '')
      : tokensNetwork.chainName
    }
    return token.symbol
  }, [tokensNetwork, token])

  const isAssetsBalanceZero = React.useMemo(() => {
    return new Amount(assetBalance).isZero()
  }, [assetBalance])

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
        <StyledWrapper isPanel={isPanel}>
          <ButtonArea disabled={isLoading} rightMargin={isAccountDetails ? 10 : 0} onClick={action}>
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
                    {
                      !isPanel &&
                      tokensNetwork &&
                      checkIfTokenNeedsNetworkIcon(tokensNetwork, token.contractAddress) &&
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

                {!isNonFungibleToken &&
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
          </ButtonArea>
          {isAccountDetails &&
            <SellButtonRow>
              {isSellSupported && !isAssetsBalanceZero &&
                <SellButton onClick={showSellModal}>{getLocale('braveWalletSell')}</SellButton>
              }
            </SellButtonRow>
          }
        </StyledWrapper>
      }
    </>
  )
}

export default PortfolioAssetItem
