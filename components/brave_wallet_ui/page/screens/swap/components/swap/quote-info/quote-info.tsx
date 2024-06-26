// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Types
import { BraveWallet } from '../../../../../../constants/types'
import { LiquiditySource, QuoteOption } from '../../../constants/types'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetSwapSupportedNetworksQuery,
  useGetTokenSpotPricesQuery
} from '../../../../../../common/slices/api.slice'
import {
  useAccountsQuery //
} from '../../../../../../common/slices/api.slice.extra'

import {
  querySubscriptionOptions60s //
} from '../../../../../../common/slices/constants'

// Selectors
import {
  useSafeUISelector //
} from '../../../../../../common/hooks/use-safe-selector'
import { UISelectors } from '../../../../../../common/selectors'

// Hooks
import {
  useBalancesFetcher //
} from '../../../../../../common/hooks/use-balances-fetcher'

// Constants
import LPMetadata from '../../../constants/LpMetadata'

// Utils
import Amount from '../../../../../../utils/amount'
import { getLocale } from '../../../../../../../common/locale'
import {
  formatDateAsRelative //
} from '../../../../../../utils/datetime-utils'
import { getBalance } from '../../../../../../utils/balance-utils'
import {
  getTokenPriceAmountFromRegistry,
  getTokenPriceFromRegistry //
} from '../../../../../../utils/pricing-utils'
import {
  getPriceIdForToken //
} from '../../../../../../utils/api-utils'

// Components
import {
  PopupModal //
} from '../../../../../../components/desktop/popup-modals/index'
import {
  SelectAccount //
} from '../../../../composer_ui/select_token_modal/select_account/select_account'
import {
  BottomSheet //
} from '../../../../../../components/shared/bottom_sheet/bottom_sheet'

// Styled Components
import {
  BraveFeeDiscounted,
  Bubble,
  Button,
  LPIcon,
  LPSeparator,
  LPRow,
  Section,
  CaratDownIcon,
  FreeText,
  CaratRightIcon,
  ExpandRow,
  ExpandButton
} from './quote-info.style'
import {
  Text,
  Row,
  Column,
  HorizontalSpace
} from '../../../../../../components/shared/style'

const getLPIcon = (source: LiquiditySource) => {
  if (source.icon) {
    return `chrome://image?${source.icon}`
  }
  return LPMetadata[source.name] ?? ''
}

interface Props {
  selectedQuoteOption: QuoteOption | undefined
  fromToken: BraveWallet.BlockchainToken | undefined
  toToken: BraveWallet.BlockchainToken | undefined
  isBridge: boolean
  onChangeRecipient: (address: string) => void

  toAccount?: BraveWallet.AccountInfo
  swapFees?: BraveWallet.SwapFees
}

export const QuoteInfo = (props: Props) => {
  const {
    selectedQuoteOption,
    fromToken,
    toToken,
    swapFees,
    isBridge,
    toAccount,
    onChangeRecipient
  } = props

  // State
  const [showProviders, setShowProviders] = React.useState<boolean>(false)
  const [showAccountSelector, setShowAccountSelector] =
    React.useState<boolean>(false)
  const [showAdvancedInformation, setShowAdvancedInformation] =
    React.useState<boolean>(false)

  // Selectors
  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // Queries
  const { accounts } = useAccountsQuery()

  const { data: networks = [] } = useGetSwapSupportedNetworksQuery()

  const { data: tokenBalancesRegistry, isLoading: isLoadingBalances } =
    useBalancesFetcher({
      accounts,
      networks
    })

  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    !isLoadingBalances && toToken && fromToken && defaultFiatCurrency
      ? {
          ids: [getPriceIdForToken(toToken), getPriceIdForToken(fromToken)],
          toCurrency: defaultFiatCurrency
        }
      : skipToken,
    querySubscriptionOptions60s
  )

  // Methods
  const handleSelectAccount = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      onChangeRecipient(account.accountId.address)
      setShowAccountSelector(false)
    },
    [onChangeRecipient]
  )

  // Memos & Computed
  const toTokenPriceAmount =
    spotPriceRegistry &&
    toToken &&
    getTokenPriceAmountFromRegistry(spotPriceRegistry, toToken)

  const swapRate: string = React.useMemo(() => {
    if (selectedQuoteOption === undefined) {
      return ''
    }

    return getLocale('braveWalletExchangeFor')
      .replace('$1', `1 ${selectedQuoteOption.fromToken.symbol}`)
      .replace(
        '$2',
        `${selectedQuoteOption.rate.format(6)} ${
          selectedQuoteOption.toToken.symbol
        }`
      )
  }, [selectedQuoteOption])

  const coinGeckoDelta: Amount = React.useMemo(() => {
    if (
      fromToken !== undefined &&
      spotPriceRegistry &&
      !getTokenPriceAmountFromRegistry(
        spotPriceRegistry,
        fromToken
      ).isUndefined() &&
      toTokenPriceAmount !== undefined &&
      selectedQuoteOption !== undefined
    ) {
      // Exchange rate is the value <R> in the following equation:
      // 1 FROM = <R> TO

      // CoinGecko rate computation:
      //   1 FROM = <R> TO
      //   1 FROM/USD = <R> TO/USD
      //   => <R> = (FROM/USD) / (TO/USD)
      const coinGeckoRate = getTokenPriceAmountFromRegistry(
        spotPriceRegistry,
        fromToken
      ).div(toTokenPriceAmount)

      // Quote rate computation:
      //   <X> FROM = <Y> TO
      //   1 FROM = <R> TO
      //   => <R> = <Y>/<X>
      const quoteRate = selectedQuoteOption.rate

      // The trade is profitable if quoteRate > coinGeckoRate.
      return quoteRate.minus(coinGeckoRate).div(quoteRate).times(100)
    }

    return Amount.zero()
  }, [spotPriceRegistry, fromToken, selectedQuoteOption, toTokenPriceAmount])

  const coinGeckoDeltaText: string = React.useMemo(() => {
    if (coinGeckoDelta.gte(0)) {
      return getLocale('braveSwapCoinGeckoCheaper').replace(
        '$1',
        coinGeckoDelta.format(2)
      )
    }

    if (coinGeckoDelta.gte(-1)) {
      return getLocale('braveSwapCoinGeckoWithin').replace(
        '$1',
        coinGeckoDelta.times(-1).format(2)
      )
    }

    return getLocale('braveSwapCoinGeckoExpensive').replace(
      '$1',
      coinGeckoDelta.times(-1).format(2)
    )
  }, [coinGeckoDelta])

  const coinGeckoDeltaColor = React.useMemo(() => {
    if (coinGeckoDelta.gte(-1)) {
      return 'success'
    }

    if (coinGeckoDelta.gte(-5)) {
      return 'warning'
    }

    return 'error'
  }, [coinGeckoDelta])

  const swapImpact: string = React.useMemo(() => {
    if (selectedQuoteOption === undefined) {
      return ''
    }
    return selectedQuoteOption.impact.format(6)
  }, [selectedQuoteOption])

  const minimumReceived: string = React.useMemo(() => {
    if (
      selectedQuoteOption === undefined ||
      selectedQuoteOption.minimumToAmount === undefined
    ) {
      return ''
    }

    return selectedQuoteOption.minimumToAmount.formatAsAsset(
      6,
      selectedQuoteOption.toToken.symbol
    )
  }, [selectedQuoteOption])

  const braveFee: BraveWallet.SwapFees | undefined = React.useMemo(() => {
    if (!swapFees) {
      return undefined
    }

    return {
      ...swapFees,
      effectiveFeePct: new Amount(swapFees.effectiveFeePct).format(6),
      discountPct: new Amount(swapFees.discountPct).format(6),
      feePct: new Amount(swapFees.feePct).format(6)
    }
  }, [swapFees])

  const estimatedDuration = React.useMemo(() => {
    if (!selectedQuoteOption?.executionDuration) {
      return ''
    }
    const date = new Date()
    // Converts executionDuration to milliseconds and creates a
    // past date from now to use as a relative time of execution.
    date.setTime(
      date.getTime() - 1000 * Number(selectedQuoteOption.executionDuration)
    )
    return formatDateAsRelative(date, undefined, true)
  }, [selectedQuoteOption])

  const accountsForReceivingToken = React.useMemo(() => {
    if (!toToken) {
      return []
    }
    return accounts
      .filter((account) => account.accountId.coin === toToken.coin)
      .sort(function (a, b) {
        return new Amount(
          getBalance(b.accountId, toToken, tokenBalancesRegistry)
        )
          .minus(getBalance(a.accountId, toToken, tokenBalancesRegistry))
          .toNumber()
      })
  }, [accounts, toToken, tokenBalancesRegistry])

  const AccountSelector = React.useMemo(() => {
    if (!toToken) {
      return
    }
    return (
      <SelectAccount
        token={toToken}
        accounts={accountsForReceivingToken}
        tokenBalancesRegistry={tokenBalancesRegistry}
        spotPrice={
          spotPriceRegistry
            ? getTokenPriceFromRegistry(spotPriceRegistry, toToken)
            : undefined
        }
        onSelectAccount={handleSelectAccount}
      />
    )
  }, [
    toToken,
    accountsForReceivingToken,
    tokenBalancesRegistry,
    spotPriceRegistry,
    handleSelectAccount
  ])

  const effectiveFeeAmount = braveFee && new Amount(braveFee.effectiveFeePct)

  return (
    <Column fullWidth={true}>
      <Section
        fullWidth={true}
        margin='16px 0px'
        padding='10px 16px'
        gap='8px'
      >
        {!isBridge && (
          <Row justifyContent='space-between'>
            <Text
              textSize='12px'
              textColor='secondary'
            >
              {getLocale('braveWalletExchangeRate')}
            </Text>
            <Text
              textSize='12px'
              isBold={true}
              textColor='primary'
            >
              {swapRate}
            </Text>
          </Row>
        )}

        {(isBridge || showAdvancedInformation) &&
          selectedQuoteOption &&
          selectedQuoteOption.sources.length > 0 && (
            <Column fullWidth={true}>
              <Row justifyContent='space-between'>
                <Text
                  textSize='12px'
                  textColor='secondary'
                  textAlign='left'
                >
                  {isBridge
                    ? getLocale('braveWalletBridgingVia')
                    : getLocale('braveWalletSwappingVia')}
                </Text>
                <Row width='unset'>
                  <LPIcon icon={LPMetadata[selectedQuoteOption.provider]} />
                  <Text
                    textSize='12px'
                    isBold={true}
                    textColor='primary'
                  >
                    {selectedQuoteOption.provider}
                  </Text>
                  <HorizontalSpace space='8px' />
                  <Button onClick={() => setShowProviders((prev) => !prev)}>
                    <CaratDownIcon isOpen={showProviders} />
                  </Button>
                </Row>
              </Row>
              {showProviders && (
                <LPRow
                  justifyContent='flex-start'
                  padding='8px 0px 0px 0px'
                >
                  {selectedQuoteOption.sources.map((source, idx) => (
                    <Row
                      key={source.name}
                      width='unset'
                    >
                      {getLPIcon(source) !== '' ? (
                        <LPIcon icon={getLPIcon(source)} />
                      ) : null}
                      <Text
                        isBold={true}
                        textSize='12px'
                        textColor='primary'
                      >
                        {source.name.split('_').join(' ')}
                      </Text>

                      {idx !== selectedQuoteOption.sources.length - 1 && (
                        <LPSeparator
                          textSize='12px'
                          textColor='secondary'
                        >
                          {selectedQuoteOption.routing === 'split' ? '+' : '×'}
                        </LPSeparator>
                      )}
                    </Row>
                  ))}
                </LPRow>
              )}
            </Column>
          )}

        {braveFee && (
          <Row justifyContent='space-between'>
            <Text
              textSize='12px'
              textColor='secondary'
            >
              {getLocale('braveSwapBraveFee')}
            </Text>
            <Row
              width='unset'
              gap='4px'
            >
              {effectiveFeeAmount && effectiveFeeAmount.isZero() ? (
                <Bubble padding='5px 6px'>
                  <FreeText
                    textSize='10px'
                    isBold={true}
                  >
                    {getLocale('braveSwapFree')}
                  </FreeText>
                </Bubble>
              ) : (
                <Text textSize='14px'>{braveFee.effectiveFeePct}%</Text>
              )}

              {braveFee.discountCode !== BraveWallet.SwapDiscountCode.kNone && (
                <>
                  <BraveFeeDiscounted
                    textSize='12px'
                    isBold={true}
                    textColor='primary'
                  >
                    {braveFee.feePct}%
                  </BraveFeeDiscounted>

                  {effectiveFeeAmount && effectiveFeeAmount.gt(0) && (
                    <Text
                      textSize='12px'
                      textColor='primary'
                    >
                      (-{braveFee.discountPct}%)
                    </Text>
                  )}
                </>
              )}
            </Row>
          </Row>
        )}

        {showAdvancedInformation && (
          <>
            {estimatedDuration !== '' && (
              <Row justifyContent='space-between'>
                <Text
                  textSize='12px'
                  textColor='secondary'
                >
                  {getLocale('braveWalletEstTime')}
                </Text>
                <Text
                  textSize='12px'
                  isBold={true}
                  textColor='primary'
                >
                  {estimatedDuration}
                </Text>
              </Row>
            )}

            {minimumReceived !== '' && (
              <Row justifyContent='space-between'>
                <Text
                  textSize='12px'
                  textColor='secondary'
                  textAlign='left'
                >
                  {getLocale('braveSwapMinimumReceivedAfterSlippage')}
                </Text>
                <Text
                  textSize='12px'
                  isBold={true}
                  textColor='primary'
                  textAlign='right'
                >
                  {minimumReceived}
                </Text>
              </Row>
            )}

            <Row justifyContent='space-between'>
              <Text
                textSize='12px'
                textColor='secondary'
                textAlign='left'
              >
                {getLocale('braveSwapPriceImpact')}
              </Text>
              <Text
                textSize='12px'
                isBold={true}
                textColor='primary'
                textAlign='right'
              >
                {swapImpact === '0' ? `${swapImpact}%` : `~ ${swapImpact}%`}
              </Text>
            </Row>

            <Row justifyContent='flex-end'>
              <Text
                textSize='12px'
                textColor={coinGeckoDeltaColor}
              >
                {coinGeckoDeltaText}
              </Text>
            </Row>

            {isBridge && (
              <Row justifyContent='space-between'>
                <Text
                  textSize='12px'
                  textColor='secondary'
                  textAlign='left'
                >
                  {getLocale('braveWalletRecipient')}
                </Text>
                <Button onClick={() => setShowAccountSelector(true)}>
                  <Text
                    textSize='12px'
                    isBold={true}
                    textColor='primary'
                  >
                    {toAccount?.name ?? ''}
                  </Text>
                  <HorizontalSpace space='8px' />
                  <CaratRightIcon />
                </Button>
              </Row>
            )}
          </>
        )}

        <ExpandRow>
          <ExpandButton
            onClick={() => setShowAdvancedInformation((prev) => !prev)}
          >
            <CaratDownIcon isOpen={showAdvancedInformation} />
          </ExpandButton>
        </ExpandRow>
      </Section>

      {selectedQuoteOption && (
        <Section
          fullWidth={true}
          margin='0px 0px 16px 0px'
          padding='10px 16px'
        >
          <Row justifyContent='space-between'>
            <Text
              textSize='12px'
              textColor='secondary'
            >
              {getLocale('braveWalletEstFees')}
            </Text>
            <Text
              textSize='12px'
              isBold={true}
              textColor='primary'
            >
              ~{selectedQuoteOption.networkFeeFiat}
            </Text>
          </Row>
        </Section>
      )}
      {isPanel && (
        <BottomSheet
          onClose={() => setShowAccountSelector(false)}
          isOpen={showAccountSelector}
        >
          {AccountSelector}
        </BottomSheet>
      )}

      {!isPanel && showAccountSelector && (
        <PopupModal
          title=''
          onClose={() => setShowAccountSelector(false)}
          width='560px'
          showDivider={false}
        >
          {AccountSelector}
        </PopupModal>
      )}
    </Column>
  )
}
