// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { getLocale } from '../../../../common/locale'
import Loading from '../../../components/loading'
import { TradingDropdown } from '../../shared'
import * as S from '../../shared/styles'
import * as FTXActions from '../ftx_actions'
import { FTXState } from '../ftx_state'

type Props = {
  ftx: FTXState
  actions: typeof FTXActions
}

type Choices = {
  from: string
  to: string
  quantity: number
}

const doNothing = () => { console.debug('Action doNothing was fired') }

function ConversionInProgress (props: Props) {
  const data = props.ftx.conversionInProgress
  // Validate
  if (!data) {
    return null
  }
  const isFetchingQuote = !data.quote
  // 1 minute countdown
  const [timeLeft, setTimeLeft] = React.useState(60)
  // This should run every time timeLeft changes, which is every second
  React.useEffect(() => {
    // Do nothing if the user clicked submit
    if (data.isSubmitting || isFetchingQuote) {
      return
    }
    setTimeout(() => {
      // Cancel if we're at 0
      if (timeLeft === 0) {
        props.actions.cancelConversion()
        return
      }
      // Count down
      setTimeLeft(timeLeft - 1)
    }, 1000)
  }, [timeLeft, setTimeLeft, data.isSubmitting, isFetchingQuote])
  // Countdown label
  const countdownLabel = React.useMemo(() => {
    return getLocale('ftxConversionConfirmLabel').replace('$1', `${timeLeft}`)
  }, [ timeLeft ])

  return (
    <S.BasicBox $mt={15} $mb={10} isFlex={true} column={true} alignItems={'stretch'}>
      <S.Text>
        {getLocale('ftxConversionPreviewTitle')}
      </S.Text>
      <S.BasicBox $mb={20}>
        <S.BasicBox isFlex={true}>
          <S.LightText small={true}>{getLocale('ftxConversionQuantityLabel')}</S.LightText>
          <S.Text small={true}>{data.quote ? data.quote.cost : data.quantity} {data.from}</S.Text>
        </S.BasicBox>
        {data.quote &&
        <>
        <S.BasicBox isFlex={true}>
          <S.LightText small={true}>{getLocale('ftxConversionPriceLabel')}</S.LightText>
          <S.Text small={true}>{data.quote.price} {data.to}</S.Text>
        </S.BasicBox>
        <S.BasicBox isFlex={true}>
          <S.LightText small={true}>{getLocale('ftxConversionProceedsLabel')}</S.LightText>
          <S.Text small={true}>{data.quote.proceeds} {data.to}</S.Text>
        </S.BasicBox>
        </>
        }
        {isFetchingQuote &&
          <Loading />
        }
      </S.BasicBox>
      {!isFetchingQuote &&
      <S.ActionButton disabled={data.isSubmitting} onClick={!data.isSubmitting ? props.actions.submitConversion : doNothing}>
        {data.isSubmitting
          ? <>{getLocale('ftxConversionSubmittingLabel')}</>
          : <>{countdownLabel}</>
        }
      </S.ActionButton>
      }
      <S.PlainButton onClick={props.actions.cancelConversion} $mt={8}>
        {getLocale('ftxConversionCancelLabel')}
      </S.PlainButton>

    </S.BasicBox>
  )
}

function ConversionSuccessful (props: Props) {
  return (
    <S.BasicBox isFlex={true} column={true} $mt={14} $mb={25} $gap={10}>
      <S.Text $fontSize={24}>🎉</S.Text>
      <S.Text>{getLocale('ftxConversionSuccessTitle')}</S.Text>
      <S.ActionButton onClick={props.actions.cancelConversion}>
        {getLocale('ftxConversionDoneLabel')}
      </S.ActionButton>
    </S.BasicBox>
  )
}

export default function Convert (props: Props) {
  const [choices, setChoices] = React.useState<Choices>({ from: '', to: '', quantity: 0 })

  const onChange = React.useCallback((from: string, to: string, quantity: number) => {
    setChoices({ from, to, quantity })
  }, [setChoices])

  const availableAmount = React.useMemo(() => {
    const amount = props.ftx.balances[choices.from] || 0
    return Number(amount.toFixed(4))
  }, [choices.from])

  const previewAction = React.useCallback(() => {
    props.actions.previewConversion({
      from: choices.from,
      to: choices.to,
      quantity: choices.quantity
    })
  }, [choices, props.actions.previewConversion])

  // Handle user has no balance
  const hasBalances = !!props.ftx.balanceTotal
  if (!hasBalances) {
    return (
      <>
        <S.BasicBox isFlex={true} $mt={15} $mb={25}>
          <S.Text>{getLocale('ftxConversionBalanceNeeded')}</S.Text>
        </S.BasicBox>
      </>
    )
  }

  // Handle conversion in progress
  const hasConversionInProgress = !!props.ftx.conversionInProgress
  if (hasConversionInProgress) {
    // Handle Success
    if (props.ftx.conversionInProgress?.complete) {
      return <ConversionSuccessful ftx={props.ftx} actions={props.actions} />
    }
    return <ConversionInProgress ftx={props.ftx} actions={props.actions} />
  }

  return (
    <>
      <S.BasicBox isFlex={true} $mt={15}>
        <S.Text>
          {getLocale('ftxConvert')}
        </S.Text>
        <S.Text small={true}>
          {
            getLocale('ftxConversionAmountAvailable')
              .replace('$1', `${availableAmount}`)
              .replace('$2', choices.from)
          }
        </S.Text>
      </S.BasicBox>
      <TradingDropdown
        fromAssets={Object.keys(props.ftx.balances)}
        toAssets={props.ftx.currencyNames}
        onChange={onChange}
      />
      <S.ActionsWrapper>
        <S.ActionButton onClick={previewAction}>
          {getLocale('ftxConversionPreviewLabel')}
        </S.ActionButton>
      </S.ActionsWrapper>
    </>
  )
}
