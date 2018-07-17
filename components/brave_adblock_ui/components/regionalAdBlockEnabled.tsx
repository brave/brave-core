/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

interface Props {
  regionalAdBlockEnabled: boolean
  regionalAdBlockTitle: string
}

export const RegionalAdBlockEnabled = (props: Props) => (
  <div>
    <span i18n-content='regionalAdblockEnabledTitle'/>&nbsp;
    {
      props.regionalAdBlockEnabled
      ? <span i18n-content='regionalAdblockEnabled'/>
      : <span i18n-content='regionalAdblockDisabled'/>
    }
    <div>
    {
      props.regionalAdBlockEnabled
      ? props.regionalAdBlockTitle
      : null
    }
    </div>
  </div>
)
