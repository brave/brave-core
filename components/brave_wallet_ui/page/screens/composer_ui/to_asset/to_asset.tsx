// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'

// Utils
import { getLocale } from '../../../../../common/locale'
import { computeFiatAmount } from '../../../../utils/pricing-utils'
import { getPriceIdForToken } from '../../../../utils/api-utils'
import Amount from '../../../../utils/amount'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetTokenSpotPricesQuery
} from '../../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s //
} from '../../../../common/slices/constants'

// Types
import { BraveWallet, SendPageTabHashes } from '../../../../constants/types'

// Components
import { SelectButton } from '../select_button/select_button'
import {
  LoadingSkeleton //
} from '../../../../components/shared/loading-skeleton/index'

// Styled Components
import {
  NetworkAndFiatText,
  ReceiveAndQuoteText,
  NetworkAndFiatRow,
  ReceiveAndQuoteRow,
  SelectAndInputRow
} from './to_asset.style'
import { AmountInput, ToSectionWrapper } from '../shared_composer.style'
import { Column, Row } from '../../../../components/shared/style'

interface Props {
  onClickSelectToken: () => void
  onInputChange: (value: string) => void
  inputValue: string
  inputDisabled: boolean
  hasInputError: boolean
  token: BraveWallet.BlockchainToken | undefined
  network: BraveWallet.NetworkInfo | undefined
  selectedSendOption: SendPageTabHashes
  isFetchingQuote: boolean
  children?: React.ReactNode
}

export const ToAsset = (props: Props) => {
  const {
    token,
    onClickSelectToken,
    onInputChange: onChange,
    hasInputError,
    inputDisabled,
    inputValue,
    network,
    selectedSendOption,
    isFetchingQuote,
    children
  } = props

  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

  const { data: spotPriceRegistry, isFetching: isLoadingSpotPrices } =
    useGetTokenSpotPricesQuery(
      token && defaultFiatCurrency
        ? {
            ids: [getPriceIdForToken(token)],
            toCurrency: defaultFiatCurrency
          }
        : skipToken,
      querySubscriptionOptions60s
    )

  // methods
  const onInputChange = React.useCallback(
    (event: React.ChangeEvent<HTMLInputElement>) => {
      onChange(event.target.value)
    },
    [onChange]
  )

  const fiatValue = React.useMemo(() => {
    if (!token || selectedSendOption === SendPageTabHashes.nft) {
      return ''
    }

    return computeFiatAmount({
      spotPriceRegistry,
      value: new Amount(inputValue !== '' ? inputValue : '0')
        .multiplyByDecimals(token.decimals)
        .toHex(),
      token: token
    }).formatAsFiat(defaultFiatCurrency)
  }, [
    spotPriceRegistry,
    token,
    inputValue,
    defaultFiatCurrency,
    selectedSendOption
  ])

  // render
  return (
    <ToSectionWrapper fullWidth={true}>
      <Column
        fullWidth={true}
        justifyContent='space-between'
        alignItems='center'
        padding='48px 0px 0px 0px'
      >
        <ReceiveAndQuoteRow
          width='100%'
          alignItems='center'
          justifyContent='space-between'
          padding='0px 16px'
          marginBottom={10}
        >
          <ReceiveAndQuoteText
            textSize='16px'
            isBold={false}
          >
            {getLocale('braveWalletReceiveEstimate')}
          </ReceiveAndQuoteText>
          {isFetchingQuote && (
            <ReceiveAndQuoteText
              textSize='16px'
              isBold={false}
            >
              {getLocale('braveSwapFindingPrice')}
            </ReceiveAndQuoteText>
          )}
        </ReceiveAndQuoteRow>
        <SelectAndInputRow
          width='100%'
          alignItems='center'
          justifyContent='space-between'
          padding='0px 16px 0px 6px'
          marginBottom={10}
        >
          <Row width='unset'>
            <SelectButton
              onClick={onClickSelectToken}
              token={token}
              selectedSendOption={selectedSendOption}
              placeholderText={getLocale('braveWalletChooseAsset')}
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
          padding='0px 16px'
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
    </ToSectionWrapper>
  )
}
