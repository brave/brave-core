/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { Locale, LocaleContext, formatMessage } from '../../shared/lib/locale_context'
import { TipKind } from '../lib/interfaces'
import { HostContext } from '../lib/host_context'

import { CustomAmountInput } from './custom_amount_input'
import { CustomTipAmount } from './custom_tip_amount'
import { ExchangeAmount } from './exchange_amount'
import { TipAmountSelector, TipAmountOption } from './tip_amount_selector'
import { BatString } from './bat_string'
import { TermsOfService } from '../../shared/components/terms_of_service'
import { NewTabLink } from '../../shared/components/new_tab_link'
import { FormSubmitButton } from './form_submit_button'

import { SadFaceIcon } from './icons/sad_face'
import { PaperAirplaneIcon } from './icons/paper_airplane_icon'
import { CalendarIcon } from './icons/calendar_icon'

import * as style from './bat_tip_form.style'

const minimumTip = 0.25
const maximumTip = 100
const tipAmountStep = 0.25

function getInsufficientFundsMessage (locale: Locale, onlyAnon: boolean) {
  const { getString } = locale

  if (onlyAnon) {
    return <>{getString('notEnoughPoints')}</>
  }

  return (
    <>
      {
        formatMessage(getString('notEnoughTokens'), {
          tags: {
            $1: (content) => (
              <NewTabLink key='add' href='chrome://rewards/#add-funds'>
                {content}
              </NewTabLink>
            )
          }
        })
      }
    </>
  )
}

interface Props {
  tipKind: TipKind
  tipAmountOptions: TipAmountOption[]
  userBalance: number
  defaultTipAmount: number
  onSubmitTip: (tipAmount: number) => void
}

export function BatTipForm (props: Props) {
  const host = React.useContext(HostContext)
  const locale = React.useContext(LocaleContext)
  const { getString } = locale

  const [rewardsParameters, setRewardsParameters] = React.useState(
    host.state.rewardsParameters)
  const [onlyAnon, setOnlyAnon] = React.useState(
    Boolean(host.state.onlyAnonWallet))

  const [tipAmount, setTipAmount] = React.useState(props.defaultTipAmount)
  const [showCustomInput, setShowCustomInput] = React.useState(false)
  const [hasCustomAmount, setHasCustomAmount] = React.useState(false)

  React.useEffect(() => {
    return host.addListener((state) => {
      setRewardsParameters(state.rewardsParameters)
      setOnlyAnon(Boolean(state.onlyAnonWallet))
    })
  }, [host])

  // Select a default tip amount
  React.useEffect(() => {
    if (tipAmount === 0 && !showCustomInput) {
      setTipAmount(props.defaultTipAmount)
    }
  }, [props.defaultTipAmount])

  if (!rewardsParameters) {
    return null
  }

  const onShowCustomInputClick = () => {
    setShowCustomInput(true)
  }

  const hideCustomInput = () => {
    setTipAmount(props.defaultTipAmount)
    setShowCustomInput(false)
  }

  const onSubmitCustomTip = () => {
    setShowCustomInput(false)
    setHasCustomAmount(true)
  }

  const onResetCustomAmount = () => {
    setTipAmount(props.defaultTipAmount)
    setHasCustomAmount(false)
  }

  const onSubmitTip = () => {
    props.onSubmitTip(tipAmount)
  }

  return (
    <style.root>
      <style.main>
        {
          showCustomInput ?
            <style.customInput>
              <CustomAmountInput
                amount={tipAmount}
                exchangeRate={rewardsParameters.rate}
                amountStep={tipAmountStep}
                maximumAmount={maximumTip}
                onAmountChange={setTipAmount}
                onHideInput={hideCustomInput}
              />
            </style.customInput>
          : hasCustomAmount ?
            <style.customAmount>
              <CustomTipAmount
                text={
                  getString(props.tipKind === 'one-time'
                    ? 'customTipText'
                    : 'customMonthlyTipText')
                }
                amount={tipAmount}
                currency={<BatString />}
                exchangeAmount={
                  <ExchangeAmount
                    amount={tipAmount}
                    rate={rewardsParameters.rate}
                  />
                }
                onReset={onResetCustomAmount}
              />
            </style.customAmount>
          :
            <style.amounts>
              <TipAmountSelector
                options={props.tipAmountOptions}
                selectedValue={tipAmount}
                onSelect={setTipAmount}
              />
              <style.customAmountButton>
                <button
                  data-test-id='custom-tip-button'
                  onClick={onShowCustomInputClick}
                >
                  {getString('customTipAmount')}
                </button>
              </style.customAmountButton>
            </style.amounts>
        }
      </style.main>
      <style.footer>
        <style.terms>
          <TermsOfService />
        </style.terms>
        {
          tipAmount < minimumTip ?
            <style.minimumAmount>
              <SadFaceIcon />&nbsp;
              {
                formatMessage(getString('minimumTipAmount'), [
                  minimumTip.toFixed(2),
                  <BatString key='currency' />
                ])
              }
            </style.minimumAmount>
          : props.tipKind === 'one-time' && tipAmount > props.userBalance ?
            <style.notEnoughFunds>
              <SadFaceIcon /> {getInsufficientFundsMessage(locale, onlyAnon)}
            </style.notEnoughFunds>
          : showCustomInput ?
            <FormSubmitButton onClick={onSubmitCustomTip}>
              {getString('continue')}
            </FormSubmitButton>
          :
            <FormSubmitButton onClick={onSubmitTip}>
              <span className={`submit-${props.tipKind}`}>
                {
                  props.tipKind === 'monthly'
                    ? <><CalendarIcon /> {getString('doMonthly')}</>
                    : <><PaperAirplaneIcon /> {getString('sendDonation')}</>
                }
              </span>
            </FormSubmitButton>
        }
      </style.footer>
    </style.root>
  )
}
