// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useSelector } from 'react-redux'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Options
import { BraveWallet, WalletState } from '../../../constants/types'

// Utils
import Amount from '../../../utils/amount'
import { getLocale } from '../../../../common/locale'
import { unbiasedRandom } from '../../../utils/random-utils'
import { checkIfTokenNeedsNetworkIcon } from '../../../utils/asset-utils'
import {
  getIsRewardsToken,
  getNormalizedExternalRewardsNetwork,
  getRewardsTokenDescription
} from '../../../utils/rewards_utils'
import {
  externalWalletProviderFromString
} from '../../../../brave_rewards/resources/shared/lib/external_wallet'

// Hooks
import { useGetNetworkQuery } from '../../../common/slices/api.slice'
import {
  useOnClickOutside
} from '../../../common/hooks/useOnClickOutside'

// Components
import {
  withPlaceholderIcon //
} from '../../shared/create-placeholder-icon/index'
import { CreateNetworkIcon } from '../../shared/create-network-icon/index'
import { LoadingSkeleton } from '../../shared/loading-skeleton/index'
import {
  WithHideBalancePlaceholder //
} from '../with-hide-balance-placeholder/index'
import { NftIcon } from '../../shared/nft-icon/nft-icon'
import {
  AssetItemMenu
} from '../wallet-menus/asset-item-menu'
import { RewardsMenu } from '../wallet-menus/rewards_menu'

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
  ButtonArea,
  AssetMenuWrapper,
  AssetMenuButton,
  AssetMenuButtonIcon
} from './style'
import {
  IconsWrapper,
  NetworkIconWrapper
} from '../../shared/style'

interface Props {
  action?: () => void
  assetBalance: string
  token: BraveWallet.BlockchainToken
  account?: BraveWallet.AccountInfo
  hideBalances?: boolean
  isPanel?: boolean
  spotPrice: string
}

const ICON_CONFIG = { size: 'medium', marginLeft: 0, marginRight: 8 } as const
const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, ICON_CONFIG)
const NftIconWithPlaceholder = withPlaceholderIcon(NftIcon, ICON_CONFIG)

export const PortfolioAssetItem = ({
  assetBalance,
  action,
  token,
  hideBalances,
  isPanel,
  spotPrice,
  account
}: Props) => {
  // redux
  const defaultCurrencies = useSelector(({ wallet }: { wallet: WalletState }) => wallet.defaultCurrencies)

  // queries
  const { data: tokensNetwork } = useGetNetworkQuery(token ?? skipToken)

  // state
  const [assetNameSkeletonWidth, setAssetNameSkeletonWidth] = React.useState(0)
  const [assetNetworkSkeletonWidth, setAssetNetworkSkeletonWidth] = React.useState(0)
  const [showAssetMenu, setShowAssetMenu] = React.useState<boolean>(false)

  // refs
  const assetMenuRef = React.useRef<HTMLDivElement>(null)

  // hooks
  useOnClickOutside(
    assetMenuRef,
    () => setShowAssetMenu(false),
    showAssetMenu
  )

  // memos & computed
  const isNonFungibleToken = token.isNft

  const formattedAssetBalance = isNonFungibleToken
    ? new Amount(assetBalance)
      .divideByDecimals(token.decimals)
      .format()
    : new Amount(assetBalance)
      .divideByDecimals(token.decimals)
      .formatAsAsset(6, token.symbol)

  const fiatBalance = React.useMemo(() => {
    if (!spotPrice) {
      return Amount.empty()
    }

    return new Amount(assetBalance)
      .divideByDecimals(token.decimals)
      .times(spotPrice)
  }, [spotPrice, assetBalance, token.chainId])

  const formattedFiatBalance = React.useMemo(() => {
    return fiatBalance.formatAsFiat(defaultCurrencies.fiat)
  }, [fiatBalance, defaultCurrencies.fiat])

  const isLoading = formattedAssetBalance === '' && !isNonFungibleToken

  const isRewardsToken = getIsRewardsToken(token)

  const externalProvider =
    isRewardsToken
      ? externalWalletProviderFromString(token.chainId)
      : null

  const NetworkDescription = React.useMemo(() => {
    if (isRewardsToken) {
      return getRewardsTokenDescription(externalProvider)
    }

    if (tokensNetwork && !isPanel) {
      return token.symbol !== ''
      ? getLocale('braveWalletPortfolioAssetNetworkDescription')
        .replace('$1', token.symbol)
        .replace('$2', tokensNetwork.chainName ?? '')
      : tokensNetwork.chainName
    }
    return token.symbol
  }, [
    tokensNetwork,
    token,
    isRewardsToken,
    externalProvider
  ])

  const network =
    isRewardsToken
      ? getNormalizedExternalRewardsNetwork(externalProvider)
      : tokensNetwork

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
          <ButtonArea
            disabled={isLoading}
            rightMargin={10}
            onClick={action}
          >
            <NameAndIcon>
              <IconsWrapper>
                {!token.logo
                  ? <LoadingSkeleton
                    circle={true}
                    width={40}
                    height={40}
                  />
                  : <>
                    {
                      isNonFungibleToken
                        ? <NftIconWithPlaceholder
                            asset={token}
                            network={network}
                          />
                        : <AssetIconWithPlaceholder
                            asset={token}
                            network={network}
                          />
                    }
                    {
                      !isPanel &&
                      network &&
                      checkIfTokenNeedsNetworkIcon(
                        network,
                        token.contractAddress
                      ) &&
                      <NetworkIconWrapper>
                        <CreateNetworkIcon
                          network={network}
                          marginRight={0}
                        />
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
          <AssetMenuWrapper
            ref={assetMenuRef}
          >
            <AssetMenuButton
              onClick={() => setShowAssetMenu(prev => !prev)}
            >
              <AssetMenuButtonIcon />
            </AssetMenuButton>
            {showAssetMenu &&
              <>
                {isRewardsToken ? (
                  <RewardsMenu />
                ) : (
                  <AssetItemMenu
                    assetBalance={assetBalance}
                    asset={token}
                    account={account}
                  />
                )}
              </>
            }
          </AssetMenuWrapper>
        </StyledWrapper>
      }
    </>
  )
}

export default PortfolioAssetItem
