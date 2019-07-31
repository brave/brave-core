/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  BlockedInfoRowText,
  BlockedInfoRowData,
  BlockedInfoRowDetails,
  BlockedInfoRowSummary,
  ArrowUpIcon,
  ArrowDownIcon,
  BlockedInfoRowStats,
  BlockedListStatic
} from 'brave-ui/features/shields'

// Group components
import StaticResourcesList from '../../shared/resourcesBlockedList/staticResourcesList'

// Locale
import { getLocale } from '../../../background/api/localeAPI'

// Helpers
import {
  blockedResourcesSize,
  shouldDisableResourcesRow
} from '../../../helpers/shieldsUtils'

// Types
import { BlockJSOptions } from '../../../types/other/blockTypes'
import { NoScriptInfo } from '../../../types/other/noScriptInfo'

interface Props {
  javascript: BlockJSOptions
  javascriptBlocked: number
  noScriptInfo: NoScriptInfo
}

interface State {
  scriptsBlockedOpen: boolean
}

export default class AdsTrackersControl extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = { scriptsBlockedOpen: false }
  }

  get shouldDisableResourcesRow (): boolean {
    const { javascriptBlocked } = this.props
    return shouldDisableResourcesRow(javascriptBlocked)
  }

  get javascriptBlockedDisplay (): string {
    const { javascriptBlocked } = this.props
    return blockedResourcesSize(javascriptBlocked)
  }

  triggerOpenScriptsBlocked = () => {
    if (!this.shouldDisableResourcesRow) {
      this.setState({ scriptsBlockedOpen: !this.state.scriptsBlockedOpen })
    }
  }

  render () {
    const { scriptsBlockedOpen } = this.state
    const { noScriptInfo } = this.props

    return (
      <BlockedInfoRowDetails>
        <BlockedInfoRowSummary onClick={this.triggerOpenScriptsBlocked}>
          <BlockedInfoRowData disabled={this.shouldDisableResourcesRow}>
            {
              scriptsBlockedOpen
                ? <ArrowUpIcon />
                : <ArrowDownIcon />
            }
            <BlockedInfoRowStats>{this.javascriptBlockedDisplay}</BlockedInfoRowStats>
            <BlockedInfoRowText>
              <span>{getLocale('scriptsBlocked')}</span>
            </BlockedInfoRowText>
          </BlockedInfoRowData>
        </BlockedInfoRowSummary>
        <BlockedListStatic>
          <StaticResourcesList list={Object.keys(noScriptInfo)} />
        </BlockedListStatic>
      </BlockedInfoRowDetails>
    )
  }
}
