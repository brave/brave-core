/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { LocaleContext } from '../../lib/locale_context'
import { WarningIcon } from './icons/warning_icon'

import * as style from './bap_deprecation_popup.style'

interface Props {
  onLearnMore: () => void
}

export function BAPDeprecationPopup (props: Props) {
  const { getString } = React.useContext(LocaleContext)
  return (
    <style.root>
      <style.content>
        <style.heading>
          <WarningIcon /> {getString('bapDeprecationHeader')}
        </style.heading>
        <style.text>
          {getString('bapDeprecationPopupText')}
        </style.text>
        <style.action>
          <button onClick={props.onLearnMore}>
            {getString('bapDeprecationLearnMore')}
          </button>
        </style.action>
      </style.content>
    </style.root>
  )
}
