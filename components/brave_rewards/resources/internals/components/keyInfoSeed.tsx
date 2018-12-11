/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Utils
import { getLocale } from '../../../../common/locale'

interface Props {
  isKeyInfoSeedValid: string
}

const getKeyInfoSeedValidString = (isValid: boolean) => {
  if (isValid) {
    return getLocale('valid')
  }

  return getLocale('invalid')
}

export const KeyInfoSeed = (props: Props) => (
  <div>
    <span i18n-content='keyInfoSeed'/> {getKeyInfoSeedValidString(props.isKeyInfoSeedValid === 'true')}
  </div>
)
