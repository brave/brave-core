/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  BlockedInfoRowDetails,
  BlockedInfoRowSummary,
  BlockedInfoRowData,
  ArrowUpIcon,
  ArrowDownIcon,
  BlockedInfoRowStats,
  BlockedInfoRowText,
  BlockedListStatic,
  dummyData
} from '../../../../components'

// Group components
import StaticResourcesList from '../../shared/resourcesBlockedList/staticResourcesList'

// Helpers
import { getLocale } from '../../../fakeLocale'

interface State {
  dummyThirdPartyFingerprintingBlockedOpen: boolean
}

export default class AdsTrackersControl extends React.PureComponent<{}, State> {
  constructor (props: {}) {
    super(props)
    this.state = { dummyThirdPartyFingerprintingBlockedOpen: false }
  }

  onClickFakeThirdPartyFingerprintingBlocked = () => {
    this.setState({ dummyThirdPartyFingerprintingBlockedOpen: !this.state.dummyThirdPartyFingerprintingBlockedOpen })
  }
  render () {
    const { dummyThirdPartyFingerprintingBlockedOpen } = this.state
    return (
      <BlockedInfoRowDetails>
        <BlockedInfoRowSummary onClick={this.onClickFakeThirdPartyFingerprintingBlocked}>
          <BlockedInfoRowData>
            {
              dummyThirdPartyFingerprintingBlockedOpen
                ? <ArrowUpIcon />
                : <ArrowDownIcon />
            }
            <BlockedInfoRowStats>{2}</BlockedInfoRowStats>
            <BlockedInfoRowText>
              <span>{getLocale('thirdPartyFingerprintingBlocked')}</span>
            </BlockedInfoRowText>
          </BlockedInfoRowData>
        </BlockedInfoRowSummary>
        <BlockedListStatic>
          <StaticResourcesList list={dummyData.otherBlockedResources} />
        </BlockedListStatic>
      </BlockedInfoRowDetails>
    )
  }
}
