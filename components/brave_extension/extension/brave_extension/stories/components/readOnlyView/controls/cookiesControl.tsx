/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  BlockedInfoRowSingleText,
  BlockedInfoRowText
} from '../../../../components'

// Helpers
import { getLocale } from '../../../fakeLocale'

export default class AdsTrackersControl extends React.PureComponent<{}, {}> {
  render () {
    return (
      <BlockedInfoRowSingleText>
        <BlockedInfoRowText>
          <span>{getLocale('thirdPartyCookiesBlocked')}</span>
        </BlockedInfoRowText>
      </BlockedInfoRowSingleText>
    )
  }
}
