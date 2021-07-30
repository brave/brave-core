/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext, formatMessage } from '../../lib/locale_context'
import { NewTabLink } from '../new_tab_link'
import { TermsOfService } from '../terms_of_service'
import { BatIcon } from '../icons/bat_icon'
import { MainButton } from './main_button'

import * as style from './settings_opt_in_form.style'

interface Props {
  onEnable: () => void
  onTakeTour: () => void
}

export function SettingsOptInForm (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  return (
    <style.root>
      <style.heading>
        <BatIcon />{getString('onboardingBraveRewards')}
      </style.heading>
      <style.subHeading>
        {getString('onboardingEarnHeader')}
      </style.subHeading>
      <style.text>
        {getString('onboardingEarnText')}&nbsp;
        {
          formatMessage(getString('onboardingDetailLinks'), {
            tags: {
              $1: (content) => (
                <a key='tour' href='#' onClick={props.onTakeTour}>{content}</a>
              ),
              $3: (content) => (
                <NewTabLink key='learn' href='https://basicattentiontoken.org'>
                  {content}
                </NewTabLink>
              )
            }
          })
        }
      </style.text>
      <style.enable>
        <MainButton onClick={props.onEnable}>
          {getString('onboardingStartUsingRewards')}
        </MainButton>
      </style.enable>
      <style.footer>
        <TermsOfService />
      </style.footer>
    </style.root>
  )
}
