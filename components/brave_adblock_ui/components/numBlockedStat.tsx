/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

interface Props {
  adsBlockedStat: number
}

export const NumBlockedStat = (props: Props) => (
  <div>
    <span i18n-content='adsBlocked'/> {props.adsBlockedStat || 0}
  </div>
)
