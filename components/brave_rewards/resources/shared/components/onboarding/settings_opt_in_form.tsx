/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { NewTabLink } from '../new_tab_link'
import { TermsOfService } from '../terms_of_service'
import { BatIcon } from '../icons/bat_icon'
import { OptInIcon } from './icons/optin_icon'
import { MainButton } from './main_button'

import * as style from './settings_opt_in_form.style'

import * as urls from '../../lib/rewards_urls'

interface Props {
  onEnable?: () => void
}

export function SettingsOptInForm (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  return (
    <style.root>
      <style.icon>
        <OptInIcon />
      </style.icon>
      <style.heading>
        {getString('onboardingEarnHeader')}
      </style.heading>
      <style.text>
        {getString('onboardingEarnText')}
      </style.text>
      <style.enable>
        {
          props.onEnable
            ? <MainButton onClick={props.onEnable}>
                {getString('onboardingStartUsingRewards')}
              </MainButton>
            : formatMessage(getString('onboardingStartUsingRewardsTextOnly'), [
                <BatIcon key='icon' />
              ])
        }
      </style.enable>
      <style.learnMore>
        <NewTabLink href={urls.rewardsTourURL}>
          {getString('rewardsLearnMore')}
        </NewTabLink>
      </style.learnMore>
      <style.terms>
        <TermsOfService />
      </style.terms>
    </style.root>
  )
}
