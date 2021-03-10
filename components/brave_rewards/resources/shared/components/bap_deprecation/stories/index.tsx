/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'

import { LocaleContext } from '../../../lib/locale_context'
import { WithThemeVariables } from '../../with_theme_variables'

import { BAPDeprecationPopup } from '../bap_deprecation_popup'
import { BAPDeprecationAlert } from '../bap_deprecation_alert'

import { localeStrings } from './locale_strings'

const localeContext = {
  getString (key: string) {
    return localeStrings[key] || 'MISSING'
  }
}

function actionLogger (name: string) {
  return (...args: any[]) => {
    console.log(name, ...args)
  }
}

interface StoryWrapperProps {
  children: React.ReactNode
}

function StoryWrapper (props: StoryWrapperProps) {
  return (
    <LocaleContext.Provider value={localeContext}>
      <WithThemeVariables>
        {props.children}
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}

storiesOf('Rewards/BAP Deprecation', module)
  .add('Popup', () => {
    return (
      <StoryWrapper>
        <BAPDeprecationPopup onLearnMore={actionLogger('onLearnMore')} />
      </StoryWrapper>
    )
  })
  .add('Alert', () => {
    return (
      <StoryWrapper>
        <BAPDeprecationAlert onClose={actionLogger('onClose')} />
      </StoryWrapper>
    )
  })
