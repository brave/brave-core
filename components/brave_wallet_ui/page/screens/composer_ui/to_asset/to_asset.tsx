// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Utils
import { getLocale, formatLocale } from '$web-common/locale'
import {
  computeFiatAmount,
  getPriceRequestsForTokens,
} from '../../../../utils/pricing-utils'
import Amount from '../../../../utils/amount'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetTokenSpotPricesQuery,
} from '../../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s, //
} from '../../../../common/slices/constants'

// Types
import { BraveWallet, SendPageTabHashes } from '../../../../constants/types'
import { SwapParamsOverrides } from '../../swap/constants/types'

// Components
import { SelectButton } from '../select_button/select_button'
import {
  LoadingSkeleton, //
} from '../../../../components/shared/loading-skeleton/index'

// Styled Components
import {
  NetworkAndFiatText,
  ReceiveAndQuoteText,
  NetworkAndFiatRow,
  ReceiveAndQuoteRow,
  SelectAndInputRow,
  RefreshIcon,
  RefreshButton,
} from './to_asset.style'
import { AmountInput } from '../shared_composer.style'
import { Column, Row, Text } from '../../../../components/shared/style'

const makeTwoDigits = (n: number) => {
  return (n < 10 ? '0' : '') + n
}

const millisecondToString = (milliseconds: number) => {
  const calculatedMin = Math.floor(milliseconds / 60000)
  const calculatedSec = Math.floor(milliseconds / 1000)
  const secRemaining = calculatedSec - calculatedMin * 60
  const min = makeTwoDigits(calculatedMin)
  const sec = makeTwoDigits(secRemaining)
  return `${min}:${sec}`
}

interface Props {
  onClickSelectToken: () => void
  onInputChange: (value: string) => void
  onRefreshQuote: (overrides: SwapParamsOverrides) => void
  inputValue: string
  inputDisabled: boolean
  buttonDisabled?: boolean
  hasInputError: boolean
  token: BraveWallet.BlockchainToken | undefined
  network: BraveWallet.NetworkInfo | undefined
  selectedSendOption: SendPageTabHashes
  isFetchingQuote: boolean
  timeUntilNextQuote?: number
  children?: React.ReactNode
}

export const ToAsset = (props: Props) => {
  const {
    token,
    onClickSelectToken,
    onRefreshQuote,
    onInputChange: onChange,
    hasInputError,
    inputDisabled,
    buttonDisabled,
    inputValue,
    network,
    selectedSendOption,
    isFetchingQuote,
    timeUntilNextQuote,
    children,
  } = props

  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

  const tokenPriceRequests = React.useMemo(
    () => getPriceRequestsForTokens([token]),
    [token],
  )

  const { data: spotPrices = [], isFetching: isLoadingSpotPrices } =
    useGetTokenSpotPricesQuery(
      tokenPriceRequests.length && defaultFiatCurrency
        ? {
            requests: tokenPriceRequests,
            vsCurrency: defaultFiatCurrency,
          }
        : skipToken,
      querySubscriptionOptions60s,
    )

  // State
  const [refreshClicked, setRefreshClicked] = React.useState<boolean>(false)

  // methods
  const onInputChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      onChange(event.target.value)
    },
    [onChange],
  )

  const handleRefreshQuote = React.useCallback(() => {
    setRefreshClicked(true)
    onRefreshQuote({})
  }, [onRefreshQuote])

  // Memos
  const fiatValue = React.useMemo(() => {
    if (!token || selectedSendOption === SendPageTabHashes.nft) {
      return ''
    }

    return computeFiatAmount({
      spotPrices,
      value: new Amount(inputValue !== '' ? inputValue : '0')
        .multiplyByDecimals(token.decimals)
        .toHex(),
      token: token,
    }).formatAsFiat(defaultFiatCurrency)
  }, [spotPrices, token, inputValue, defaultFiatCurrency, selectedSendOption])

  // Computed
  const countdown =
    timeUntilNextQuote !== undefined
      ? millisecondToString(timeUntilNextQuote)
      : ''

  const newQuote = formatLocale('braveWalletNewQuoteIn', {
    $1: (
      <Text
        textSize='12px'
        isBold
        textColor='primary'
      >
        {countdown}
      </Text>
    ),
  })

  // render
  return (
    <Column
      fullWidth={true}
      fullHeight={true}
      justifyContent='flex-start'
      alignItems='center'
      padding='32px 0px 0px 0px'
    >
      <ReceiveAndQuoteRow
        width='100%'
        alignItems='center'
        justifyContent='space-between'
        marginBottom={10}
      >
        <ReceiveAndQuoteText
          textSize='14px'
          isBold={false}
        >
          {getLocale('braveWalletReceiveEstimate')}
        </ReceiveAndQuoteText>
        <Row width='unset'>
          {!isFetchingQuote && timeUntilNextQuote !== undefined && (
            <Row
              width='unset'
              gap='4px'
              margin='0px 4px 0px 0px'
            >
              <ReceiveAndQuoteText
                textSize='12px'
                isBold={false}
              >
                {newQuote}
              </ReceiveAndQuoteText>
            </Row>
          )}
          {timeUntilNextQuote !== undefined && (
            <RefreshButton
              onClick={handleRefreshQuote}
              clicked={refreshClicked}
              onAnimationEnd={() => setRefreshClicked(false)}
              disabled={refreshClicked || isFetchingQuote}
            >
              <RefreshIcon />
            </RefreshButton>
          )}
        </Row>
      </ReceiveAndQuoteRow>
      <SelectAndInputRow
        width='100%'
        alignItems='center'
        justifyContent='space-between'
        marginBottom={10}
      >
        <Row width='unset'>
          <SelectButton
            onClick={onClickSelectToken}
            token={token}
            selectedSendOption={selectedSendOption}
            placeholderText={getLocale('braveWalletChooseAsset')}
            disabled={buttonDisabled}
          />
        </Row>
        <AmountInput
          placeholder='0.0'
          type='number'
          spellCheck={false}
          onChange={onInputChange}
          value={inputValue}
          hasError={hasInputError}
          disabled={inputDisabled}
        />
      </SelectAndInputRow>
      <NetworkAndFiatRow
        width='100%'
        alignItems='center'
        justifyContent='space-between'
      >
        {network && token && (
          <NetworkAndFiatText
            textSize='14px'
            isBold={false}
          >
            {getLocale('braveWalletPortfolioAssetNetworkDescription')
              .replace('$1', '')
              .replace('$2', network.chainName)}
          </NetworkAndFiatText>
        )}
        {token && (
          <Row width='unset'>
            {isLoadingSpotPrices ? (
              <LoadingSkeleton
                height={18}
                width={60}
              />
            ) : (
              <NetworkAndFiatText
                textSize='14px'
                isBold={false}
              >
                {fiatValue}
              </NetworkAndFiatText>
            )}
          </Row>
        )}
      </NetworkAndFiatRow>
      {children}
    </Column>
  )
}
