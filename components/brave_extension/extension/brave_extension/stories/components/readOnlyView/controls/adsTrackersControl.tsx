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
} from '../../../../components'

// Group components
import StaticResourcesList from '../../shared/resourcesBlockedList/staticResourcesList'

// Helpers
import { getLocale } from '../../../fakeLocale'

interface State {
  dummyThirdPartyTrackersBlockedOpen: boolean
}

export default class AdsTrackersControl extends React.PureComponent<{}, State> {
  constructor (props: {}) {
    super(props)
    this.state = { dummyThirdPartyTrackersBlockedOpen: false }
  }
  onClickFakeThirdPartyTrackersBlocked = () => {
    this.setState({ dummyThirdPartyTrackersBlockedOpen: !this.state.dummyThirdPartyTrackersBlockedOpen })
  }
  render () {
    const { dummyThirdPartyTrackersBlockedOpen } = this.state
    return (
      <BlockedInfoRowDetails>
        <BlockedInfoRowSummary onClick={this.onClickFakeThirdPartyTrackersBlocked}>
          <BlockedInfoRowData>
            {
              dummyThirdPartyTrackersBlockedOpen
                ? <ArrowUpIcon />
                : <ArrowDownIcon />
            }
            <BlockedInfoRowStats>{2}</BlockedInfoRowStats>
            <BlockedInfoRowText>{getLocale('thirdPartyTrackersBlocked')}</BlockedInfoRowText>
          </BlockedInfoRowData>
        </BlockedInfoRowSummary>
        <BlockedListStatic>
          <StaticResourcesList list={dummyData.otherBlockedResources} />
        </BlockedListStatic>
      </BlockedInfoRowDetails>
    )
  }
}
