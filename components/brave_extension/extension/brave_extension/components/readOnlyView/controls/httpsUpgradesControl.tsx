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
  BlockedListStatic,
  dummyData
} from 'brave-ui/features/shields'

// Group components
import StaticResourcesList from '../../shared/resourcesBlockedList/staticResourcesList'

// Helpers
import { getLocale } from '../../../background/api/localeAPI'

interface State {
  dummyConnectionsUpgradedHTTPSOpen: boolean
}

export default class AdsTrackersControl extends React.PureComponent<{}, State> {
  constructor (props: {}) {
    super(props)
    this.state = { dummyConnectionsUpgradedHTTPSOpen: false }
  }

  onClickFakeConnectionsUpgradedHTTPS = () => {
    this.setState({ dummyConnectionsUpgradedHTTPSOpen: !this.state.dummyConnectionsUpgradedHTTPSOpen })
  }

  render () {
    const { dummyConnectionsUpgradedHTTPSOpen } = this.state
    return (
      <BlockedInfoRowDetails>
        <BlockedInfoRowSummary onClick={this.onClickFakeConnectionsUpgradedHTTPS}>
          <BlockedInfoRowData disabled={false}>
            {
              dummyConnectionsUpgradedHTTPSOpen
                ? <ArrowUpIcon />
                : <ArrowDownIcon />
            }
            <BlockedInfoRowStats>{2}</BlockedInfoRowStats>
            <BlockedInfoRowText>{getLocale('connectionsUpgradedHTTPS')}</BlockedInfoRowText>
          </BlockedInfoRowData>
        </BlockedInfoRowSummary>
        <BlockedListStatic>
          <StaticResourcesList list={dummyData.otherBlockedResources} />
        </BlockedListStatic>
      </BlockedInfoRowDetails>
    )
  }
}
