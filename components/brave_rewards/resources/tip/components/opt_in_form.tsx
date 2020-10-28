/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { EmoteSadIcon } from 'brave-ui/components/icons'

import { HostContext } from '../lib/host_context'
import { LocaleContext } from '../../shared/lib/locale_context'

import { TipOptInForm } from '../../shared/components/onboarding'

import * as style from './opt_in_form.style'

export function OptInForm () {
  const host = React.useContext(HostContext)
  const { getString } = React.useContext(LocaleContext)

  const onEnable = () => host.saveOnboardingResult('opted-in')
  const onDismiss = () => host.saveOnboardingResult('dismissed')

  return (
    <style.root>
      <style.topBar>
        <style.sadIcon><EmoteSadIcon /></style.sadIcon>
        {getString('optInRequired')}
      </style.topBar>
      <style.content>
        <TipOptInForm onEnable={onEnable} onDismiss={onDismiss} />
      </style.content>
    </style.root>
  )
}
