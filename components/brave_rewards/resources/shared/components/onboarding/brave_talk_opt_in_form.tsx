/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { MainButton } from './main_button'
import { NewTabLink } from '../new_tab_link'

import { CashbackIcon } from './icons/cashback_icon'
import { CheckCircleIcon } from './icons/check_circle_icon'

import * as style from './brave_talk_opt_in_form.style'

const termsOfServiceURL = 'https://basicattentiontoken.org/user-terms-of-service'
const privacyPolicyURL = 'https://brave.com/privacy/#rewards'

interface Props {
  showRewardsOnboarding: boolean
  onEnable: () => void
  onTakeTour: () => void
}

type TosProps = {
  text: string
}

function TermsOfService (props: TosProps) {
  return (
    <style.terms>
      {
        formatMessage(props.text, {
          tags: {
            $1: (content) => (
              <NewTabLink key='terms' href={termsOfServiceURL}>
                {content}
              </NewTabLink>
            ),
            $3: (content) => (
              <NewTabLink key='privacy' href={privacyPolicyURL}>
                {content}
              </NewTabLink>
            )
          }
        })
      }
    </style.terms>
  )
}

export function BraveTalkOptInForm (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  const [showStartFreeCall, setShowStartFreeCall] = React.useState(false)

  const onEnableButtonClicked = () => {
    props.onEnable()
    setShowStartFreeCall(true)
  }

  if (showStartFreeCall) {
    return (
      <style.root>
        <CheckCircleIcon />
        <style.header>
          {getString('braveTalkCanStartFreeCall')}
        </style.header>
        <style.text>
          {getString('braveTalkClickAnywhereToBraveTalk')}
        </style.text>
        <style.learn>
          {
            formatMessage(getString('braveTalkWantLearnMore'), {
              tags: {
                $1: (content) => (
                  <style.tour key='tour' onClick={props.onTakeTour}>
                    {content}
                  </style.tour>
                )
              }
            })
          }
        </style.learn>
      </style.root>
    )
  }

  if (props.showRewardsOnboarding) {
    return (
      <style.root>
        <CashbackIcon />
        <style.header>
          {getString('braveTalkTurnOnRewardsToStartCall')}
        </style.header>
        <style.text>
          {getString('braveTalkBraveRewardsDescription')}
        </style.text>
        <style.enable>
          <MainButton onClick={onEnableButtonClicked}>
            {getString('braveTalkTurnOnRewards')}
          </MainButton>
        </style.enable>
        <TermsOfService text={getString('braveTalkOptInRewardsTerms')}/>
      </style.root>
    )
  }

  return (
    <style.root>
      <CashbackIcon />
      <style.header>
        {getString('braveTalkTurnOnPrivateAdsToStartCall')}
      </style.header>
      <style.text>
        {getString('braveTalkPrivateAdsDescription')}
      </style.text>
      <style.enable>
        <MainButton onClick={onEnableButtonClicked}>
          {getString('braveTalkTurnOnPrivateAds')}
        </MainButton>
      </style.enable>
      <TermsOfService text={getString('braveTalkOptInAdsTerms')}/>
    </style.root>
  )
}
