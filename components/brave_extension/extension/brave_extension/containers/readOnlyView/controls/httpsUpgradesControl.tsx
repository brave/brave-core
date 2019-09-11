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
} from '../../../components'

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
import { BlockOptions } from '../../../types/other/blockTypes'

interface Props {
  httpsRedirected: number
  httpUpgradableResources: BlockOptions
  httpsRedirectedResources: Array<string>
}

interface State {
  connectionsUpgradedOpen: boolean
}

export default class AdsTrackersControl extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = { connectionsUpgradedOpen: false }
  }

  get shouldDisableResourcesRow (): boolean {
    const { httpsRedirected } = this.props
    return shouldDisableResourcesRow(httpsRedirected)
  }

  get httpsRedirectedDisplay (): string {
    const { httpsRedirected } = this.props
    return blockedResourcesSize(httpsRedirected)
  }

  triggerConnectionsUpgradedToHTTPS = () => {
    if (!this.shouldDisableResourcesRow) {
      this.setState({ connectionsUpgradedOpen: !this.state.connectionsUpgradedOpen })
    }
  }

  render () {
    const { connectionsUpgradedOpen } = this.state
    const { httpsRedirectedResources } = this.props

    return (
      <BlockedInfoRowDetails>
        <BlockedInfoRowSummary onClick={this.triggerConnectionsUpgradedToHTTPS}>
          <BlockedInfoRowData disabled={this.shouldDisableResourcesRow}>
            {
              connectionsUpgradedOpen
                ? <ArrowUpIcon />
                : <ArrowDownIcon />
            }
            <BlockedInfoRowStats>{this.httpsRedirectedDisplay}</BlockedInfoRowStats>
            <BlockedInfoRowText>{getLocale('connectionsUpgradedHTTPS')}</BlockedInfoRowText>
          </BlockedInfoRowData>
        </BlockedInfoRowSummary>
        <BlockedListStatic>
          <StaticResourcesList list={httpsRedirectedResources} />
        </BlockedListStatic>
      </BlockedInfoRowDetails>
    )
  }
}
