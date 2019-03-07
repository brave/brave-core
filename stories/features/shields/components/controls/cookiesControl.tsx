/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { BlockedInfoRowSingle, SelectBox } from '../../../../../src/features/shields'

// Fake data
import { getLocale } from '../../fakeLocale'

interface Props {
  isBlockedListOpen: boolean
}

export default class CookiesControl extends React.PureComponent<Props, {}> {
  render () {
    const { isBlockedListOpen } = this.props
    return (
      <>
        <BlockedInfoRowSingle>
            <SelectBox disabled={isBlockedListOpen}>
              <option value='block_third_party'>{getLocale('thirdPartyCookiesBlocked')}</option>
              <option value='block'>{getLocale('allCookiesBlocked')}</option>
              <option value='allow'>{getLocale('allCookiesAllowed')}</option>
            </SelectBox>
        </BlockedInfoRowSingle>
      </>
    )
  }
}
