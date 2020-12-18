/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { EmoteSadIcon } from 'brave-ui/components/icons'

import { LocaleContext } from '../../shared/lib/locale_context'
import { TipOptInForm } from '../../shared/components/onboarding'

import * as style from './opt_in_form.style'

interface Props {
  onTakeTour: () => void
  onEnable: () => void
  onDismiss: () => void
}

export function OptInForm (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  return (
    <style.root>
      <style.topBar>
        <style.sadIcon><EmoteSadIcon /></style.sadIcon>
        {getString('optInRequired')}
      </style.topBar>
      <style.content>
        <TipOptInForm
          onEnable={props.onEnable}
          onDismiss={props.onDismiss}
          onTakeTour={props.onTakeTour}
        />
      </style.content>
    </style.root>
  )
}
