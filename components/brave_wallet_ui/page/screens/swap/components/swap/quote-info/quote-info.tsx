// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Types
import {
  BraveWallet,
  SpotPriceRegistry
} from '../../../../../../constants/types'
import { QuoteOption } from '../../../constants/types'

// Constants
import LPMetadata from '../../../constants/LpMetadata'

// Utils
import Amount from '../../../../../../utils/amount'
import { getLocale } from '../../../../../../../common/locale'
import {
  getTokenPriceAmountFromRegistry //
} from '../../../../../../utils/pricing-utils'

// Styled Components
import {
  BraveFeeContainer,
  BraveFeeDiscounted,
  Bubble,
  ExpandButton,
  FuelTank,
  HorizontalArrows,
  LPIcon,
  LPSeparator,
  LPRow
} from './quote-info.style'
import {
  Column,
  Row,
  Text,
  VerticalSpacer,
  HorizontalSpacer,
  Icon
} from '../../shared-swap.styles'

interface Props {
  selectedQuoteOption: QuoteOption | undefined
  fromToken: BraveWallet.BlockchainToken | undefined
  toToken: BraveWallet.BlockchainToken | undefined
  toAmount: string
  spotPrices?: SpotPriceRegistry
}

export const QuoteInfo = (props: Props) => {
  const { selectedQuoteOption, fromToken, toToken, spotPrices } = props

  // State
  const [showProviders, setShowProviders] = React.useState<boolean>(false)

  // Memos
  const swapRate: string = React.useMemo(() => {
    if (selectedQuoteOption === undefined) {
      return ''
    }

    return `1 ${
      selectedQuoteOption.fromToken.symbol
    } ≈ ${selectedQuoteOption.rate.format(6)} ${
      selectedQuoteOption.toToken.symbol
    }`
  }, [selectedQuoteOption])

  const coinGeckoDelta: Amount = React.useMemo(() => {
    if (
      fromToken !== undefined &&
      toToken !== undefined &&
      spotPrices &&
      !getTokenPriceAmountFromRegistry(spotPrices, fromToken).isUndefined() &&
      !getTokenPriceAmountFromRegistry(spotPrices, toToken).isUndefined() &&
      selectedQuoteOption !== undefined
    ) {
      // Exchange rate is the value <R> in the following equation:
      // 1 FROM = <R> TO

      // CoinGecko rate computation:
      //   1 FROM = <R> TO
      //   1 FROM/USD = <R> TO/USD
      //   => <R> = (FROM/USD) / (TO/USD)
      const coinGeckoRate = getTokenPriceAmountFromRegistry(
        spotPrices,
        fromToken
      ).div(getTokenPriceAmountFromRegistry(spotPrices, toToken))

      // Quote rate computation:
      //   <X> FROM = <Y> TO
      //   1 FROM = <R> TO
      //   => <R> = <Y>/<X>
      const quoteRate = selectedQuoteOption.rate

      // The trade is profitable if quoteRate > coinGeckoRate.
      return quoteRate.minus(coinGeckoRate).div(quoteRate).times(100)
    }

    return Amount.zero()
  }, [spotPrices, fromToken, toToken, selectedQuoteOption])

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
  }, [coinGeckoDelta, getLocale])

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

  const braveFee = React.useMemo(() => {
    if (!selectedQuoteOption?.braveFee) {
      return undefined
    }

    const { braveFee: braveFeeOriginal } = selectedQuoteOption

    return {
      ...braveFeeOriginal,
      effectiveFeePct: new Amount(braveFeeOriginal.effectiveFeePct).format(6),
      protocolFeePct: new Amount(braveFeeOriginal.protocolFeePct),
      discountOnBraveFeePct: new Amount(
        braveFeeOriginal.discountOnBraveFeePct
      ).format(6),
      braveFeePct: new Amount(braveFeeOriginal.braveFeePct).format(6)
    }
  }, [selectedQuoteOption])

  return (
    <Column
      columnHeight='dynamic'
      columnWidth='full'
    >
      <VerticalSpacer size={16} />
      <Row
        rowWidth='full'
        marginBottom={10}
        horizontalPadding={16}
      >
        <Text textSize='14px'>{getLocale('braveSwapRate')}</Text>
        <Row>
          <Text textSize='14px'>{swapRate}</Text>
          <HorizontalArrows
            name='swap-horizontal'
            size={16}
          />
        </Row>
      </Row>
      <Row
        rowWidth='full'
        marginBottom={10}
        horizontalPadding={16}
      >
        <HorizontalSpacer size={1} />
        <Row>
          <Text
            textSize='14px'
            textColor={coinGeckoDeltaColor}
          >
            {coinGeckoDeltaText}
          </Text>
        </Row>
      </Row>
      <Row
        rowWidth='full'
        marginBottom={10}
        horizontalPadding={16}
      >
        <Text textSize='14px'>{getLocale('braveSwapPriceImpact')}</Text>
        <Text textSize='14px'>
          {swapImpact === '0' ? `${swapImpact}%` : `~ ${swapImpact}%`}
        </Text>
      </Row>
      {minimumReceived !== '' && (
        <Row
          rowWidth='full'
          marginBottom={8}
          horizontalPadding={16}
        >
          <Text
            textSize='14px'
            textAlign='left'
          >
            {getLocale('braveSwapMinimumReceivedAfterSlippage')}
          </Text>
          <Text
            textSize='14px'
            textAlign='right'
          >
            {minimumReceived}
          </Text>
        </Row>
      )}
      {selectedQuoteOption && selectedQuoteOption.sources.length > 0 && (
        <Column
          columnWidth='full'
          marginBottom={8}
          horizontalPadding={16}
        >
          <Row
            rowWidth='full'
            marginBottom={8}
          >
            <Text
              textSize='14px'
              textAlign='left'
            >
              {getLocale('braveSwapLiquidityProvider')}
            </Text>
            <Row>
              <Text textSize='14px'>{selectedQuoteOption.sources.length}</Text>
              <HorizontalSpacer size={8} />
              <ExpandButton
                isExpanded={showProviders}
                onClick={() => setShowProviders((prev) => !prev)}
              >
                <Icon
                  size={14}
                  name='carat-down'
                />
              </ExpandButton>
            </Row>
          </Row>
          {showProviders && (
            <LPRow
              rowWidth='full'
              horizontalAlign='flex-start'
              verticalPadding={6}
            >
              {selectedQuoteOption.sources.map((source, idx) => (
                <Row key={idx}>
                  <Bubble>
                    <Text textSize='12px'>
                      {source.name.split('_').join(' ')}
                    </Text>
                    {LPMetadata[source.name] ? (
                      <LPIcon
                        icon={LPMetadata[source.name]}
                        size={12}
                      />
                    ) : null}
                  </Bubble>

                  {idx !== selectedQuoteOption.sources.length - 1 && (
                    <LPSeparator textSize='14px'>
                      {selectedQuoteOption.routing === 'split' ? '+' : '×'}
                    </LPSeparator>
                  )}
                </Row>
              ))}
            </LPRow>
          )}
        </Column>
      )}
      {selectedQuoteOption && (
        <Row
          rowWidth='full'
          marginBottom={8}
          horizontalPadding={16}
        >
          <Text textSize='14px'>{getLocale('braveSwapNetworkFee')}</Text>
          <Bubble>
            <FuelTank
              name='search-fuel-tank'
              size={16}
            />
            <Text textSize='14px'>{selectedQuoteOption.networkFee}</Text>
          </Bubble>
        </Row>
      )}
      {braveFee && (
        <Row
          rowWidth='full'
          marginBottom={8}
          horizontalPadding={16}
        >
          <Text textSize='14px'>{getLocale('braveSwapBraveFee')}</Text>
          <Text textSize='14px'>
            <BraveFeeContainer>
              {braveFee.discountCode === BraveWallet.DiscountCode.kNone && (
                <Text textSize='14px'>{braveFee.effectiveFeePct}%</Text>
              )}

              {braveFee.discountCode !== BraveWallet.DiscountCode.kNone && (
                <>
                  {!braveFee.hasBraveFee ? (
                    <Text
                      textSize='14px'
                      textColor='success'
                      isBold={true}
                    >
                      {getLocale('braveSwapFree')}
                    </Text>
                  ) : (
                    <Text textSize='14px'>{braveFee.effectiveFeePct}%</Text>
                  )}

                  <BraveFeeDiscounted
                    textSize='14px'
                    textColor='text03'
                  >
                    {braveFee.braveFeePct}%
                  </BraveFeeDiscounted>

                  {braveFee.hasBraveFee && (
                    <Text textSize='14px'>
                      (-{braveFee.discountOnBraveFeePct}%)
                    </Text>
                  )}
                </>
              )}
            </BraveFeeContainer>
          </Text>
        </Row>
      )}

      {braveFee && !braveFee.protocolFeePct.isZero() && (
        <Row
          rowWidth='full'
          marginBottom={16}
          horizontalPadding={16}
        >
          <Text textSize='14px'>{getLocale('braveSwapProtocolFee')}</Text>
          <Text textSize='14px'>
            <BraveFeeContainer>
              <Text textSize='14px'>{braveFee.protocolFeePct.format(6)}%</Text>
            </BraveFeeContainer>
          </Text>
        </Row>
      )}
    </Column>
  )
}
