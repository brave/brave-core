/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../shared/lib/locale_context'
import { CloseIcon } from '../../shared/components/icons/close_icon'

import backgroundImage from '../assets/bitflyer_verification_bg.png'

import * as style from './bitflyer_promotion.style'

interface Props {
  onLearnMore: () => void
  onDismiss: () => void
}

export function BitflyerPromotion (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  return (
    <style.root>
      <style.close>
        <button onClick={props.onDismiss}>
          <CloseIcon />
        </button>
      </style.close>
      <style.content>
        <style.title>
          {getString('bitflyerVerificationPromoTitle')}
        </style.title>
        <style.text>
          {getString('bitflyerVerificationPromoInfo')}
        </style.text>
        <style.learnMore>
          <button onClick={props.onLearnMore}>
            {getString('promoLearnMore')}
          </button>
        </style.learnMore>
        <style.dismiss>
          <button onClick={props.onDismiss}>
            {getString('promoDismiss')}
          </button>
        </style.dismiss>
      </style.content>
      <style.image>
        <img src={`/${backgroundImage}`} />
      </style.image>
    </style.root>
  )
}
