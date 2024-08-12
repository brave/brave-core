// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query/react'
import Label from '@brave/leo/react/label'
import { PluralStringProxyImpl } from 'chrome://resources/js/plural_string_proxy.js'
import usePromise from '$web-common/usePromise'

// Types
import { QuoteOption } from '../../../constants/types'
import {
  BraveWallet,
  SwapProviderNameMapping
} from '../../../../../../constants/types'

// Constants
import { SwapProviderMetadata } from '../../../constants/metadata'

// Queries
import {
  useGetDefaultFiatCurrencyQuery,
  useGetNetworkQuery,
  useGetTokenSpotPricesQuery //
} from '../../../../../../common/slices/api.slice'
import {
  querySubscriptionOptions60s //
} from '../../../../../../common/slices/constants'

// Utils
import {
  getLocale,
  splitStringForTag
} from '../../../../../../../common/locale'
import {
  formatDateAsRelative //
} from '../../../../../../utils/datetime-utils'
import {
  computeFiatAmount,
  getPriceIdForToken
} from '../../../../../../utils/pricing-utils'
import { getLPIcon } from '../../../swap.utils'

// Components
import {
  withPlaceholderIcon //
} from '../../../../../../components/shared/create-placeholder-icon/index'
import {
  CreateNetworkIcon //
} from '../../../../../../components/shared/create-network-icon'
import { RouteStep } from './route_step'

// Styles
import {
  OptionButton,
  IconsWrapper,
  AssetIcon,
  NetworkIconWrapper,
  CaratDownIcon,
  GasFeeBubble,
  GasIcon,
  LPIconWrapper,
  ProviderIcon,
  Lines,
  StepsWrapper,
  PercentBubble,
  Triangle,
  Dot
} from './routes.style'
import { LPIcon, BankIcon } from '../../shared-swap.styles'
import { Row, Text, Column } from '../../../../../../components/shared/style'

const AssetIconWithPlaceholder = withPlaceholderIcon(AssetIcon, {
  size: 'medium',
  marginLeft: 0,
  marginRight: 0
})

interface Props {
  option: QuoteOption
  isSelected: boolean
  onClickOption: () => void
}

export const RouteOption = (props: Props) => {
  // Props
  const { option, isSelected, onClickOption } = props
  const { toToken, toAmount, fromToken, fromAmount } = option

  // State
  const [isExpanded, setIsExpanded] = React.useState<boolean>(false)

  // Queries
  const { data: toTokensNetwork } = useGetNetworkQuery(
    toToken
      ? {
          chainId: toToken.chainId,
          coin: toToken.coin
        }
      : skipToken
  )

  const { data: fromTokensNetwork } = useGetNetworkQuery(
    fromToken
      ? {
          chainId: fromToken.chainId,
          coin: fromToken.coin
        }
      : skipToken
  )

  const { data: defaultFiatCurrency } = useGetDefaultFiatCurrencyQuery()

  const { data: spotPriceRegistry } = useGetTokenSpotPricesQuery(
    toToken && defaultFiatCurrency
      ? {
          ids: [getPriceIdForToken(toToken)],
          toCurrency: defaultFiatCurrency
        }
      : skipToken,
    querySubscriptionOptions60s
  )

  // Methods
  const handleOnClick = React.useCallback(() => {
    onClickOption()
    setIsExpanded((prev) => !prev)
  }, [onClickOption])

  // Memos
  const durationTime = React.useMemo(() => {
    if (!option.executionDuration) {
      return ''
    }
    const date = new Date()
    // Converts executionDuration to milliseconds and creates a
    // past date from now to use as a relative time of execution.
    date.setTime(date.getTime() - 1000 * Number(option.executionDuration))
    return formatDateAsRelative(date, undefined, true)
  }, [option])

  const fiatValue = React.useMemo(() => {
    return computeFiatAmount({
      spotPriceRegistry,
      value: toAmount.multiplyByDecimals(toToken.decimals).toHex(),
      token: toToken
    }).formatAsFiat(defaultFiatCurrency)
  }, [spotPriceRegistry, toToken, toAmount, defaultFiatCurrency])

  // Computed
  const firstStep =
    option.sources.find((source) =>
      source.includedSteps?.some(
        (step) => step.type === BraveWallet.LiFiStepType.kCross
      )
    ) || option.sources[0]
  const firstStepName = firstStep.name
  const firstStepIcon = getLPIcon(firstStep)
  const additionalRoutesLength = option.sources.length - 1

  const { result: exchangeStepsLocale } = usePromise(
    async () =>
      PluralStringProxyImpl.getInstance().getPluralString(
        'braveWalletExchangeNamePlusSteps',
        additionalRoutesLength
      ),
    [additionalRoutesLength]
  )

  return (
    <OptionButton
      isSelected={isSelected}
      onClick={handleOnClick}
    >
      <Row
        justifyContent='space-between'
        margin='0px 0px 14px 0px'
      >
        <Row
          width='unset'
          gap='4px'
        >
          {option.tags.length > 0 &&
            option.tags.map((tag) => (
              <Label
                key={tag}
                color={tag === 'CHEAPEST' ? 'purple' : 'blue'}
              >
                {getLocale(
                  tag === 'CHEAPEST'
                    ? 'braveWalletCheapest'
                    : 'braveWalletFastest'
                )}
              </Label>
            ))}
          {durationTime && (
            <Text
              textSize='10px'
              isBold={true}
              textColor='primary'
            >
              ~ {durationTime}
            </Text>
          )}
        </Row>
        <GasFeeBubble
          width='unset'
          padding='2px 4px'
        >
          <GasIcon />
          <Text
            textSize='10px'
            isBold={true}
            textColor='primary'
          >
            {option.networkFeeFiat}
          </Text>
        </GasFeeBubble>
      </Row>
      <Row justifyContent='space-between'>
        <Row width='unset'>
          <IconsWrapper margin='0px 8px 0px 0px'>
            <AssetIconWithPlaceholder asset={toToken} />
            {toTokensNetwork && (
              <NetworkIconWrapper padding='2px'>
                <CreateNetworkIcon
                  network={toTokensNetwork}
                  marginRight={0}
                />
              </NetworkIconWrapper>
            )}
          </IconsWrapper>
          <Column alignItems='flex-start'>
            <Text
              textSize='16px'
              isBold={true}
              textColor='primary'
              textAlign='left'
            >
              {toAmount.formatAsAsset(6, toToken.symbol)}
            </Text>
            <Text
              textSize='12px'
              isBold={false}
              textColor='secondary'
              textAlign='left'
            >
              ≈ {fiatValue}
            </Text>
          </Column>
        </Row>
        <Row
          width='unset'
          gap='4px'
          justifyContent='flex-end'
        >
          {!isExpanded && (
            <Row
              width='unset'
              gap='4px'
            >
              {firstStepIcon !== '' ? (
                <LPIcon icon={firstStepIcon} />
              ) : (
                <BankIcon />
              )}
              <Text
                textSize='12px'
                isBold={true}
                textColor='secondary'
                textAlign='right'
              >
                {additionalRoutesLength !== 0 && exchangeStepsLocale
                  ? exchangeStepsLocale.replace('$1', firstStepName)
                  : firstStepName}
              </Text>
            </Row>
          )}
          <CaratDownIcon isExpanded={isExpanded} />
        </Row>
      </Row>
      {isExpanded && (
        <Column fullWidth={true}>
          <Row
            justifyContent='flex-start'
            padding='0px 0px 0px 12px'
            margin='12px 0px 4px 0px'
          >
            <Triangle />
          </Row>
          <StepsWrapper fullWidth={true}>
            <Lines />
            {Array.from(option.sources)
              .reverse()
              .map((source) => {
                const lpIcon = getLPIcon(source)
                const descriptionString = getLocale(
                  'braveWalletExchangeViaProvider'
                )
                  .replace('$5', source.name)
                  .replace('$6', SwapProviderNameMapping[option.provider] ?? '')
                const { duringTag: exchange, afterTag } = splitStringForTag(
                  descriptionString,
                  1
                )
                const { beforeTag: via, duringTag: provider } =
                  splitStringForTag(afterTag || '', 3)
                return (
                  <Column
                    fullWidth={true}
                    alignItems='flex-start'
                    margin='12px 0px 12px 4px'
                    key={source.name}
                  >
                    <Row justifyContent='flex-start'>
                      <LPIconWrapper
                        padding='2px'
                        margin='0px 12px 0px 0px'
                      >
                        {lpIcon !== '' ? (
                          <LPIcon icon={lpIcon} />
                        ) : (
                          <BankIcon />
                        )}
                        <ProviderIcon
                          size='14px'
                          icon={SwapProviderMetadata[option.provider]}
                        />
                      </LPIconWrapper>
                      <Row
                        justifyContent='flex-start'
                        width='unset'
                        gap='4px'
                      >
                        <Text
                          textSize='16px'
                          isBold={true}
                          textColor='primary'
                        >
                          {exchange}
                        </Text>
                        <Text
                          textSize='12px'
                          isBold={false}
                          textColor='tertiary'
                        >
                          {via}
                        </Text>
                        <Text
                          textSize='16px'
                          isBold={true}
                          textColor='primary'
                        >
                          {provider}
                        </Text>
                        <PercentBubble
                          padding='4px 6px'
                          margin='0px 0px 0px 8px'
                        >
                          <Text
                            textSize='10px'
                            isBold={true}
                            textColor='primary'
                          >
                            {source.proportion.times(100).format()}%
                          </Text>
                        </PercentBubble>
                      </Row>
                    </Row>
                    {source.includedSteps &&
                      Array.from(source.includedSteps)
                        .reverse()
                        .map((step) => (
                          <RouteStep
                            step={step}
                            key={step.id}
                          />
                        ))}
                  </Column>
                )
              })}
          </StepsWrapper>
          <Row
            justifyContent='flex-start'
            padding='0px 0px 0px 13px'
            margin='4px 0px 12px 0px'
          >
            <Dot />
          </Row>
          <Row justifyContent='flex-start'>
            <IconsWrapper margin='0px 8px 0px 0px'>
              <AssetIconWithPlaceholder asset={fromToken} />
              {fromTokensNetwork && (
                <NetworkIconWrapper padding='2px'>
                  <CreateNetworkIcon
                    network={fromTokensNetwork}
                    marginRight={0}
                  />
                </NetworkIconWrapper>
              )}
            </IconsWrapper>
            <Column alignItems='flex-start'>
              <Text
                textSize='16px'
                isBold={true}
                textColor='primary'
              >
                {fromAmount.formatAsAsset(6, fromToken.symbol)}
              </Text>
              <Text
                textSize='12px'
                isBold={false}
                textColor='secondary'
              >
                {getLocale('braveWalletOnNetwork').replace(
                  '$1',
                  fromTokensNetwork?.chainName ?? ''
                )}
              </Text>
            </Column>
          </Row>
        </Column>
      )}
    </OptionButton>
  )
}
