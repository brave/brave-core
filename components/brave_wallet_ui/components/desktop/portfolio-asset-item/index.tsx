// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import LeoButton from '@brave/leo/react/button'

// Options
import { BraveWallet } from '../../../constants/types'

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
  externalWalletProviderFromString //
} from '../../../../brave_rewards/resources/shared/lib/external_wallet'

// Hooks
import {
  useGetBitcoinBalancesQuery,
  useGetDefaultFiatCurrencyQuery,
  useGetNetworkQuery
} from '../../../common/slices/api.slice'
import { useOnClickOutside } from '../../../common/hooks/useOnClickOutside'

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
import { AssetItemMenu } from '../wallet-menus/asset-item-menu'
import { RewardsMenu } from '../wallet-menus/rewards_menu'
import {
  BalanceDetailsModal //
} from '../popup-modals/balance_details_modal/balance_details_modal'
import {
  EditTokenModal //
} from '../popup-modals/edit_token_modal/edit_token_modal'
import { ShieldedLabel } from '../../shared/shielded_label/shielded_label'

// Styled Components
import {
  HoverArea,
  AssetBalanceText,
  AssetName,
  BalanceColumn,
  FiatBalanceText,
  NameAndIcon,
  AssetIcon,
  NameColumn,
  Spacer,
  NetworkDescriptionText,
  Button,
  AssetMenuWrapper,
  AssetMenuButton,
  AssetMenuButtonIcon,
  Wrapper,
  InfoBar,
  LoadingRing,
  InfoText
} from './style'
import { IconsWrapper, NetworkIconWrapper, Row } from '../../shared/style'

interface Props {
  action?: () => void
  assetBalance: string
  token: BraveWallet.BlockchainToken
  account?: BraveWallet.AccountInfo
  hideBalances?: boolean
  isPanel?: boolean
  isAccountDetails?: boolean
  spotPrice: string
  isGrouped?: boolean
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
  account,
  isAccountDetails,
  isGrouped
}: Props) => {
  // queries
  const { data: defaultFiatCurrency = 'usd' } = useGetDefaultFiatCurrencyQuery()
  const { data: tokensNetwork } = useGetNetworkQuery(token ?? skipToken)
  const { data: bitcoinBalances, isLoading: isLoadingBitcoinBalances } =
    useGetBitcoinBalancesQuery(
      token?.coin === BraveWallet.CoinType.BTC && account?.accountId
        ? account.accountId
        : skipToken
    )

  // state
  const [showAssetMenu, setShowAssetMenu] = React.useState<boolean>(false)
  const [showBalanceDetailsModal, setShowBalanceDetailsModal] =
    React.useState<boolean>(false)
  const [showEditTokenModal, setShowEditTokenModal] =
    React.useState<boolean>(false)

  // refs
  const assetMenuRef = React.useRef<HTMLDivElement>(null)
  const balanceDetailsRef = React.useRef<HTMLDivElement>(null)

  // hooks
  useOnClickOutside(assetMenuRef, () => setShowAssetMenu(false), showAssetMenu)
  useOnClickOutside(
    balanceDetailsRef,
    () => setShowBalanceDetailsModal(false),
    showBalanceDetailsModal
  )

  // memos & computed
  const isNonFungibleToken = token.isNft

  const formattedAssetBalance = isNonFungibleToken
    ? new Amount(assetBalance).divideByDecimals(token.decimals).format()
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
  }, [spotPrice, assetBalance, token.decimals])

  const formattedFiatBalance = fiatBalance.formatAsFiat(defaultFiatCurrency)

  const isLoading = formattedAssetBalance === '' && !isNonFungibleToken

  const isRewardsToken = getIsRewardsToken(token)

  const externalProvider = isRewardsToken
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
  }, [isRewardsToken, tokensNetwork, isPanel, token.symbol, externalProvider])

  const network = isRewardsToken
    ? getNormalizedExternalRewardsNetwork(externalProvider)
    : tokensNetwork

  const hasPendingBalance = !new Amount(
    bitcoinBalances?.pendingBalance ?? '0'
  ).isZero()

  const showBalanceInfo =
    hasPendingBalance && account && token.coin === BraveWallet.CoinType.BTC

  const assetNameSkeletonWidth = React.useMemo(
    () => unbiasedRandom(100, 250),
    []
  )

  const assetNetworkSkeletonWidth = React.useMemo(
    () => unbiasedRandom(100, 250),
    []
  )

  // render
  return (
    <>
      <Wrapper
        fullWidth={true}
        showBorder={isAccountDetails && showBalanceInfo}
        isGrouped={isGrouped}
      >
        {token.visible && (
          <HoverArea isGrouped={isGrouped}>
            <Button
              disabled={isLoading}
              rightMargin={10}
              onClick={action}
            >
              <NameAndIcon>
                <IconsWrapper>
                  {isNonFungibleToken ? (
                    <NftIconWithPlaceholder asset={token} />
                  ) : (
                    <AssetIconWithPlaceholder asset={token} />
                  )}
                  {!isPanel &&
                    network &&
                    checkIfTokenNeedsNetworkIcon(
                      network,
                      token.contractAddress
                    ) && (
                      <NetworkIconWrapper>
                        <CreateNetworkIcon
                          network={network}
                          marginRight={0}
                        />
                      </NetworkIconWrapper>
                    )}
                </IconsWrapper>
                <NameColumn>
                  {!token.name && !token.symbol ? (
                    <>
                      <LoadingSkeleton
                        width={assetNameSkeletonWidth}
                        height={18}
                      />
                      <Spacer />
                      <LoadingSkeleton
                        width={assetNetworkSkeletonWidth}
                        height={18}
                      />
                    </>
                  ) : (
                    <>
                      <Row
                        width='unset'
                        gap='6px'
                      >
                        <AssetName
                          textSize='14px'
                          isBold={true}
                          textAlign='left'
                        >
                          {token.isShielded ? 'Zcash' : token.name}
                        </AssetName>
                        {token.isShielded && <ShieldedLabel />}
                      </Row>
                      <NetworkDescriptionText
                        textSize='12px'
                        isBold={false}
                        textAlign='left'
                      >
                        {NetworkDescription}
                      </NetworkDescriptionText>
                    </>
                  )}
                </NameColumn>
              </NameAndIcon>
              <BalanceColumn>
                <WithHideBalancePlaceholder
                  size='small'
                  hideBalances={hideBalances ?? false}
                >
                  {formattedAssetBalance ? (
                    <AssetBalanceText
                      textSize='14px'
                      isBold={true}
                      textAlign='right'
                    >
                      {formattedAssetBalance}
                    </AssetBalanceText>
                  ) : (
                    <>
                      <LoadingSkeleton
                        width={60}
                        height={18}
                      />
                      <Spacer />
                    </>
                  )}
                  {!isNonFungibleToken && (
                    <>
                      {formattedFiatBalance ? (
                        <FiatBalanceText
                          textSize='12px'
                          isBold={false}
                          textAlign='right'
                        >
                          {formattedFiatBalance}
                        </FiatBalanceText>
                      ) : (
                        <LoadingSkeleton
                          width={60}
                          height={18}
                        />
                      )}
                    </>
                  )}
                </WithHideBalancePlaceholder>
              </BalanceColumn>
            </Button>
            <AssetMenuWrapper ref={assetMenuRef}>
              <AssetMenuButton
                onClick={() => setShowAssetMenu((prev) => !prev)}
              >
                <AssetMenuButtonIcon />
              </AssetMenuButton>
              {showAssetMenu && (
                <>
                  {isRewardsToken ? (
                    <RewardsMenu />
                  ) : (
                    <AssetItemMenu
                      assetBalance={assetBalance}
                      asset={token}
                      account={account}
                      onClickEditToken={
                        token.contractAddress !== ''
                          ? () => setShowEditTokenModal(true)
                          : undefined
                      }
                    />
                  )}
                </>
              )}
            </AssetMenuWrapper>
          </HoverArea>
        )}
        {showBalanceInfo && (
          <Row padding='8px'>
            <InfoBar justifyContent='space-between'>
              <Row
                width='unset'
                gap='16px'
              >
                <LoadingRing />
                <InfoText
                  textSize='14px'
                  isBold={false}
                  textAlign='left'
                >
                  {getLocale('braveWalletUnavailableBalances')}
                </InfoText>
              </Row>
              <div>
                <LeoButton
                  kind='plain'
                  size='tiny'
                  onClick={() => setShowBalanceDetailsModal(true)}
                >
                  {getLocale('braveWalletAllowSpendDetailsButton')}
                </LeoButton>
              </div>
            </InfoBar>
          </Row>
        )}
      </Wrapper>
      {showBalanceDetailsModal && (
        <BalanceDetailsModal
          ref={balanceDetailsRef}
          onClose={() => setShowBalanceDetailsModal(false)}
          token={token}
          isLoadingBalances={isLoadingBitcoinBalances}
          balances={bitcoinBalances}
        />
      )}
      {showEditTokenModal && (
        <EditTokenModal
          token={token}
          onClose={() => setShowEditTokenModal(false)}
        />
      )}
    </>
  )
}

export default PortfolioAssetItem
