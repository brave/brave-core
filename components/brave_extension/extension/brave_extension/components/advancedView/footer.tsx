/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Types

import { ToggleAdvancedView } from '../../types/actions/shieldsPanelActions'

// Feature-specific components
import { MainFooter, Link } from 'brave-ui/features/shields'

// API
import * as tabsAPI from '../../background/api/tabsAPI'

// Locale
import { getLocale } from '../../background/api/localeAPI'

export interface Props {
  isBlockedListOpen: boolean
  toggleAdvancedView: ToggleAdvancedView
}

export default class Footer extends React.PureComponent<Props, {}> {
  openSettings = () => {
    tabsAPI.createTab({ url: 'chrome://settings/shields' })
      .catch((err) => console.log('[Unable to open a new tab from Shields]', err))
  }

  render () {
    const { isBlockedListOpen, toggleAdvancedView } = this.props
    return (
      <MainFooter>
        <Link disabled={isBlockedListOpen} onClick={toggleAdvancedView}>
          {getLocale('simpleView')}
        </Link>
        <Link
          id='braveShieldsFooter'
          onClick={this.openSettings}
          disabled={isBlockedListOpen}
        >
          {getLocale('changeDefaults')}
        </Link>
      </MainFooter>
    )
  }
}
