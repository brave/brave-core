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
import {
  useGetDefaultFiatCurrencyQuery,
  useGetNetworkQuery
} from '../../../../common/slices/api.slice'
import { reduceInt } from '../../../../utils/string-utils'

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
import {
  ShieldedLabel //
} from '../../../../components/shared/shielded_label/shielded_label'

// Styled Components
import {
  AssetIcon,
  NetworkIconWrapper,
  Button,
  IconsWrapper,
  ButtonWrapper,
  DisabledLabel,
  PercentChangeText,
  PercentChangeIcon,
  AccountsIcon,
  InfoButton,
  InfoIcon,
  TokenBalanceText,
  FiatBalanceText,
  TokenNameText,
  TokenNameRow,
  LeftSide,
  NameAndBalanceColumn
} from './token_list_item.style'
import {
  Column,
  Row,
  VerticalSpace,
  Text
} from '../../../../components/shared/style'

interface Props {
  onClick: () => void
  onViewTokenDetails: (token: BraveWallet.BlockchainToken) => void
  token: BraveWallet.BlockchainToken
  balance?: string
  spotPrice?: BraveWallet.AssetPrice
  isLoadingSpotPrice?: boolean
  disabledText?: string
  tokenHasMultipleAccounts?: boolean
  groupingLabel?: 'owned' | 'not-owned'
}

const ICON_CONFIG = { size: 'big', marginLeft: 0, marginRight: 0 } as const
const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, ICON_CONFIG)
const NftIconWithPlaceholder = withPlaceholderIcon(NftIcon, ICON_CONFIG)

export const TokenListItem = React.forwardRef<HTMLDivElement, Props>(
  (props: Props, forwardedRef) => {
    const {
      onClick,
      onViewTokenDetails,
      token,
      spotPrice,
      isLoadingSpotPrice,
      disabledText,
      tokenHasMultipleAccounts,
      groupingLabel,
      balance
    } = props

    // Queries
    const { data: tokensNetwork } = useGetNetworkQuery(token ?? skipToken)
    const { data: defaultFiatCurrency = 'usd' } =
      useGetDefaultFiatCurrencyQuery()

    // Memos
    const fiatBalance = React.useMemo(() => {
      return balance && spotPrice
        ? new Amount(balance)
            .divideByDecimals(token.decimals)
            .times(spotPrice.price)
        : Amount.empty()
    }, [balance, spotPrice, token])

    const tokenDisplayName = React.useMemo(() => {
      if (token.isNft) {
        const id = token.tokenId
          ? `#${reduceInt(new Amount(token.tokenId).format())}`
          : ''
        return `${token.name || token.symbol} ${id}`
      }
      if (token.isShielded) {
        return 'Zcash'
      }
      return token.name || token.symbol
    }, [token])

    // Computed
    const formattedFiatBalance = fiatBalance.formatAsFiat(defaultFiatCurrency)

    const tokenHasBalance = balance && new Amount(balance).gt(0)

    const isPriceDown = spotPrice
      ? Number(spotPrice.assetTimeframeChange) < 0
      : false

    // Render
    return (
      <Column
        fullWidth={true}
        ref={forwardedRef}
      >
        {groupingLabel &&
          (groupingLabel === 'owned' ? (
            <Row
              justifyContent='space-between'
              padding='10px 16px'
            >
              <Text
                textSize='12px'
                isBold={false}
                textColor='secondary'
              >
                {getLocale('braveWalletOwned')}
              </Text>
              <Text
                textSize='12px'
                isBold={false}
                textColor='secondary'
              >
                {getLocale('braveWalletAmount24H')}
              </Text>
            </Row>
          ) : (
            <Row
              justifyContent='flex-start'
              padding='10px 16px'
            >
              <Text
                textSize='12px'
                isBold={false}
                textColor='secondary'
              >
                {getLocale('braveWalletNotOwned')}
              </Text>
            </Row>
          ))}
        <ButtonWrapper
          width='100%'
          justifyContent='space-between'
        >
          <Button
            onClick={onClick}
            disabled={!!disabledText}
          >
            <LeftSide justifyContent='flex-start'>
              <IconsWrapper>
                {token.isNft || token.isErc721 || token.isErc1155 ? (
                  <NftIconWithPlaceholder asset={token} />
                ) : (
                  <AssetIconWithPlaceholder asset={token} />
                )}
                <NetworkIconWrapper>
                  <CreateNetworkIcon
                    network={tokensNetwork}
                    marginRight={0}
                  />
                </NetworkIconWrapper>
              </IconsWrapper>
              <NameAndBalanceColumn
                alignItems='flex-start'
                width='100%'
              >
                <TokenNameRow
                  justifyContent='flex-start'
                  width='100%'
                  padding='0px 8px 0px 0px'
                  gap='6px'
                >
                  <TokenNameText
                    textSize='14px'
                    isBold={true}
                    textAlign='left'
                    textColor='primary'
                  >
                    {tokenDisplayName}
                  </TokenNameText>
                  {token.isShielded && <ShieldedLabel />}
                  {disabledText && (
                    <DisabledLabel>{getLocale(disabledText)}</DisabledLabel>
                  )}
                </TokenNameRow>
                <TokenBalanceText
                  textSize='12px'
                  isBold={false}
                  textAlign='left'
                  textColor='secondary'
                >
                  {formatTokenBalanceWithSymbol(
                    balance ?? '',
                    token.decimals,
                    token.symbol
                  )}
                </TokenBalanceText>
              </NameAndBalanceColumn>
            </LeftSide>
            {!token.isNft &&
              !token.isErc721 &&
              !token.isErc1155 &&
              tokenHasBalance && (
                <Column
                  alignItems='flex-end'
                  width='25%'
                >
                  {isLoadingSpotPrice ? (
                    <>
                      <LoadingSkeleton
                        width={60}
                        height={14}
                      />
                      <VerticalSpace space='4px' />
                      <LoadingSkeleton
                        width={60}
                        height={12}
                      />
                    </>
                  ) : (
                    <>
                      <Row width='unset'>
                        {tokenHasMultipleAccounts && <AccountsIcon />}
                        <FiatBalanceText
                          textSize='14px'
                          isBold={true}
                          textAlign='right'
                          textColor='primary'
                        >
                          {formattedFiatBalance}
                        </FiatBalanceText>
                      </Row>
                      <Row width='unset'>
                        <PercentChangeIcon
                          name={
                            isPriceDown
                              ? 'arrow-diagonal-down-right'
                              : 'arrow-diagonal-up-right'
                          }
                          isDown={isPriceDown}
                        />
                        <PercentChangeText
                          textSize='12px'
                          isBold={false}
                          textAlign='right'
                          isDown={isPriceDown}
                        >
                          {`${Math.abs(
                            Number(spotPrice?.assetTimeframeChange ?? '')
                          ).toFixed(2)}%`}
                        </PercentChangeText>
                      </Row>
                    </>
                  )}
                </Column>
              )}
          </Button>
          {(!tokenHasBalance ||
            token.isNft ||
            token.isErc721 ||
            token.isErc1155) && (
            <InfoButton onClick={() => onViewTokenDetails(token)}>
              <InfoIcon />
            </InfoButton>
          )}
        </ButtonWrapper>
      </Column>
    )
  }
)

export default TokenListItem
