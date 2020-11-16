/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { storiesOf } from '@storybook/react'

import { LocaleContext } from '../../../lib/locale_context'
import { WithThemeVariables } from '../../with_theme_variables'

import { RewardsOptInModal } from '../rewards_opt_in_modal'
import { TipOptInForm } from '../tip_opt_in_form'

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
  width?: number
  height?: number
  children: React.ReactNode
}

function StoryWrapper (props: StoryWrapperProps) {
  return (
    <LocaleContext.Provider value={localeContext}>
      <WithThemeVariables>
        <div style={{ width: props.width, height: props.height }}>
          {props.children}
        </div>
      </WithThemeVariables>
    </LocaleContext.Provider>
  )
}

storiesOf('Rewards/Onboarding', module)
  .add('Opt-in Modal', () => {
    return (
      <StoryWrapper>
        <RewardsOptInModal
          onEnable={actionLogger('onEnable')}
          onClose={actionLogger('onClose')}
          onAddFunds={actionLogger('onAddFunds')}
        />
      </StoryWrapper>
    )
  })
  .add('Tip Opt-in', () => {
    return (
      <StoryWrapper width={363} height={404}>
        <TipOptInForm
          onEnable={actionLogger('onEnable')}
          onDismiss={actionLogger('onDismiss')}
        />
      </StoryWrapper>
    )
  })
