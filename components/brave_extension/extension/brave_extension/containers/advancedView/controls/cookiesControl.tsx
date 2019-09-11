/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import { BlockedInfoRowSingle, SelectBox } from '../../../components'

// Locale
import { getLocale } from '../../../background/api/localeAPI'

// Types
import { BlockCookiesOptions } from '../../../types/other/blockTypes'

export interface Props {
  isBlockedListOpen: boolean
  cookies: BlockCookiesOptions
  blockCookies: (event: string) => void
}

export default class CookiesControl extends React.PureComponent<Props, {}> {
  onChangeBlockCookies = (event: React.ChangeEvent<HTMLSelectElement>) => {
    this.props.blockCookies(event.target.value)
  }

  render () {
    const { isBlockedListOpen, cookies } = this.props
    return (
      <>
        <BlockedInfoRowSingle id='cookiesControl'>
          <SelectBox
            id='blockCookies'
            disabled={isBlockedListOpen}
            value={cookies}
            onChange={this.onChangeBlockCookies}
          >
            <option value='block_third_party'>{getLocale('thirdPartyCookiesBlocked')}</option>
            <option value='block'>{getLocale('allCookiesBlocked')}</option>
            <option value='allow'>{getLocale('allCookiesAllowed')}</option>
          </SelectBox>
        </BlockedInfoRowSingle>
      </>
    )
  }
}
