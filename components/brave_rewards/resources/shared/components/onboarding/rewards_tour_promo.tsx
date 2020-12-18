/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../lib/locale_context'
import { CloseIcon } from '../icons/close_icon'

import * as style from './rewards_tour_promo.style'

interface Props {
  onClose: () => void
  onTakeTour: () => void
}

export function RewardsTourPromo (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  return (
    <style.root>
      <style.close>
        <button onClick={props.onClose}><CloseIcon /></button>
      </style.close>
      <style.header>
        {getString('onboardingPromoHeader')}
      </style.header>
      <style.text>
        {getString('onboardingPromoText')}
      </style.text>
      <style.action>
        <button onClick={props.onTakeTour}>
          {getString('onboardingTakeTour')}
        </button>
      </style.action>
    </style.root>
  )
}
