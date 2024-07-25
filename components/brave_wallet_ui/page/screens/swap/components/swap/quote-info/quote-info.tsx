// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import { PluralStringProxyImpl } from 'chrome://resources/js/plural_string_proxy.js'
import usePromise from '$web-common/usePromise'

// Types
import { BraveWallet } from '../../../../../../constants/types'
import { QuoteOption } from '../../../constants/types'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetNetworkQuery,
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

// Utils
import Amount from '../../../../../../utils/amount'
import { getLocale } from '../../../../../../../common/locale'
import {
  formatDateAsRelative //
} from '../../../../../../utils/datetime-utils'
import { getBalance } from '../../../../../../utils/balance-utils'
import {
  getTokenPriceAmountFromRegistry,
  getTokenPriceFromRegistry,
  getPriceIdForToken
} from '../../../../../../utils/pricing-utils'
import { getLPIcon } from '../../../swap.utils'

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
import { MaxSlippage } from '../max_slippage/max_slippage'
import {
  InfoIconTooltip //
} from '../../../../../../components/shared/info_icon_tooltip/info_icon_tooltip'
import { Routes } from '../routes/routes'

// Styled Components
import {
  BraveFeeDiscounted,
  Bubble,
  Button,
  LiquidityProviderIcon,
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
import { BankIcon } from '../../shared-swap.styles'

interface Props {
  fromToken: BraveWallet.BlockchainToken | undefined
  toToken: BraveWallet.BlockchainToken | undefined
  isBridge: boolean
  slippageTolerance: string
  quoteOptions: QuoteOption[]
  selectedQuoteOptionId: string
  onSelectQuoteOption: (id: string) => void
  onChangeRecipient: (address: string) => void
  onChangeSlippageTolerance: (slippage: string) => void
  toAccount?: BraveWallet.AccountInfo
  swapFees?: BraveWallet.SwapFees
}

export const QuoteInfo = (props: Props) => {
  const {
    fromToken,
    toToken,
    swapFees,
    isBridge,
    toAccount,
    slippageTolerance,
    quoteOptions,
    selectedQuoteOptionId,
    onSelectQuoteOption,
    onChangeSlippageTolerance,
    onChangeRecipient
  } = props

  // State
  const [showAccountSelector, setShowAccountSelector] =
    React.useState<boolean>(false)
  const [showAdvancedInformation, setShowAdvancedInformation] =
    React.useState<boolean>(false)
  const [showMaxSlippage, setShowMaxSlippage] = React.useState<boolean>(false)
  const [showRoutes, setShowRoutes] = React.useState<boolean>(false)

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

  const { data: txNetwork } = useGetNetworkQuery(
    fromToken
      ? {
          chainId: fromToken.chainId,
          coin: fromToken.coin
        }
      : skipToken
  )

  // Methods
  const handleSelectAccount = React.useCallback(
    (account: BraveWallet.AccountInfo) => {
      onChangeRecipient(account.accountId.address)
      setShowAccountSelector(false)
    },
    [onChangeRecipient]
  )

  const handleOnChangeSlippageTolerance = React.useCallback(
    (slippage: string) => {
      onChangeSlippageTolerance(slippage)
      setShowMaxSlippage(false)
    },
    [onChangeSlippageTolerance]
  )

  const handleOnChangeRoute = React.useCallback(
    (id: string) => {
      onSelectQuoteOption(id)
      setShowRoutes(false)
    },
    [onSelectQuoteOption]
  )

  // Memos & Computed
  const selectedQuoteOption = quoteOptions.find(
    (option) => option.id === selectedQuoteOptionId
  )
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
  const firstStep =
    selectedQuoteOption?.sources.find((source) =>
      source.includedSteps?.some(
        (step) => step.type === BraveWallet.LiFiStepType.kCross
      )
    ) || selectedQuoteOption?.sources[0]
  const firstStepName = firstStep?.name ?? ''
  const firstStepIcon = firstStep ? getLPIcon(firstStep) : ''
  const additionalRoutesLength = selectedQuoteOption
    ? selectedQuoteOption.sources.length - 1
    : 0

  const { result: exchangeStepsLocale } = usePromise(
    async () =>
      PluralStringProxyImpl.getInstance().getPluralString(
        'braveWalletExchangeNamePlusSteps',
        additionalRoutesLength
      ),
    [additionalRoutesLength]
  )

  return (
    <Column fullWidth={true}>
      <Section
        fullWidth={true}
        margin='16px 0px'
        padding='10px 16px'
        gap='8px'
      >
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
            {getLocale('braveWalletMaxSlippage')}
          </Text>
          <Row width='unset'>
            <Text
              textSize='12px'
              isBold={true}
              textColor='primary'
              textAlign='right'
            >
              {slippageTolerance}%
            </Text>
            <HorizontalSpace space='8px' />
            <Button onClick={() => setShowMaxSlippage((prev) => !prev)}>
              <CaratDownIcon isOpen={showMaxSlippage} />
            </Button>
          </Row>
        </Row>
      </Section>

      <Section
        fullWidth={true}
        margin='0px 0px 16px 0px'
        padding='10px 16px'
        gap='8px'
      >
        {!showAdvancedInformation && selectedQuoteOption && (
          <Row justifyContent='space-between'>
            <Row
              width='unset'
              gap='8px'
            >
              {firstStepIcon ? (
                <LiquidityProviderIcon
                  icon={firstStepIcon}
                  size='16px'
                />
              ) : (
                <BankIcon size='16px' />
              )}
              <Text
                textSize='12px'
                isBold={true}
                textColor='primary'
              >
                {firstStepName}{' '}
                {additionalRoutesLength !== 0
                  ? `+ ${additionalRoutesLength}`
                  : ''}
              </Text>
            </Row>
            {txNetwork && (
              <Row
                width='unset'
                gap='8px'
              >
                <Text
                  textSize='12px'
                  textColor='secondary'
                >
                  {getLocale('braveSwapNetworkFee')}
                </Text>
                <Text
                  textSize='12px'
                  isBold={true}
                  textColor='primary'
                >
                  {selectedQuoteOption.networkFee.formatAsAsset(
                    6,
                    txNetwork.symbol
                  )}
                </Text>
              </Row>
            )}
          </Row>
        )}

        {showAdvancedInformation && (
          <>
            {selectedQuoteOption && selectedQuoteOption.sources.length > 0 && (
              <Column fullWidth={true}>
                <Row justifyContent='space-between'>
                  <Text
                    textSize='12px'
                    textColor='secondary'
                    textAlign='left'
                  >
                    {getLocale('braveWalletRoute')}
                  </Text>
                  <Row
                    width='unset'
                    gap='8px'
                  >
                    {firstStepIcon ? (
                      <LiquidityProviderIcon
                        icon={firstStepIcon}
                        size='16px'
                      />
                    ) : (
                      <BankIcon size='16px' />
                    )}
                    <Text
                      textSize='12px'
                      isBold={true}
                      textColor='primary'
                    >
                      {additionalRoutesLength !== 0 && exchangeStepsLocale
                        ? exchangeStepsLocale.replace('$1', firstStepName)
                        : firstStepName}
                    </Text>
                    <Button onClick={() => setShowRoutes((prev) => !prev)}>
                      <CaratDownIcon isOpen={showRoutes} />
                    </Button>
                  </Row>
                </Row>
              </Column>
            )}

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

            <Row justifyContent='space-between'>
              <Row
                width='unset'
                gap='4px'
              >
                <Text
                  textSize='12px'
                  textColor='secondary'
                  textAlign='left'
                >
                  {getLocale('braveSwapPriceImpact')}
                </Text>
                <InfoIconTooltip
                  placement='right'
                  text={getLocale('braveWalletPriceImpactDescription')}
                  maxContentWidth={isPanel ? '200px' : undefined}
                />
              </Row>
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

                  {braveFee.discountCode !==
                    BraveWallet.SwapDiscountCode.kNone && (
                    <>
                      <BraveFeeDiscounted
                        textSize='12px'
                        isBold={true}
                        textColor='primary'
                      >
                        {braveFee.feePct}%
                      </BraveFeeDiscounted>

                      {effectiveFeeAmount?.gt(0) && (
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

            {selectedQuoteOption && txNetwork && (
              <Row justifyContent='space-between'>
                <Text
                  textSize='12px'
                  textColor='secondary'
                >
                  {getLocale('braveSwapNetworkFee')}
                </Text>
                <Text
                  textSize='12px'
                  isBold={true}
                  textColor='primary'
                >
                  {selectedQuoteOption.networkFee.formatAsAsset(
                    6,
                    txNetwork.symbol
                  )}
                </Text>
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

      {isPanel && (
        <>
          <BottomSheet
            onClose={() => setShowRoutes(false)}
            isOpen={showRoutes}
          >
            <Routes
              quoteOptions={quoteOptions}
              onSelectQuoteOption={handleOnChangeRoute}
              selectedQuoteOptionId={selectedQuoteOptionId}
            />
          </BottomSheet>
          <BottomSheet
            onClose={() => setShowAccountSelector(false)}
            isOpen={showAccountSelector}
          >
            {AccountSelector}
          </BottomSheet>
          <BottomSheet
            onClose={() => setShowMaxSlippage(false)}
            isOpen={showMaxSlippage}
          >
            <MaxSlippage
              slippageTolerance={slippageTolerance}
              onChangeSlippageTolerance={handleOnChangeSlippageTolerance}
            />
          </BottomSheet>
        </>
      )}
      {!isPanel && (
        <>
          {showRoutes && (
            <PopupModal
              title=''
              onClose={() => setShowRoutes(false)}
              width='560px'
              showDivider={false}
            >
              <Routes
                quoteOptions={quoteOptions}
                onSelectQuoteOption={handleOnChangeRoute}
                selectedQuoteOptionId={selectedQuoteOptionId}
              />
            </PopupModal>
          )}
          {showAccountSelector && (
            <PopupModal
              title=''
              onClose={() => setShowAccountSelector(false)}
              width='560px'
              showDivider={false}
            >
              {AccountSelector}
            </PopupModal>
          )}
          {showMaxSlippage && (
            <PopupModal
              title=''
              onClose={() => setShowMaxSlippage(false)}
              width='560px'
              showDivider={false}
            >
              <MaxSlippage
                slippageTolerance={slippageTolerance}
                onChangeSlippageTolerance={handleOnChangeSlippageTolerance}
              />
            </PopupModal>
          )}
        </>
      )}
    </Column>
  )
}
