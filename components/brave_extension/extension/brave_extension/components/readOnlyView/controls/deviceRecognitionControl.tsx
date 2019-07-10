/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  BlockedInfoRowDetails,
  ArrowUpIcon,
  ArrowDownIcon,
  BlockedInfoRowStats,
  BlockedInfoRowForSelectSummary,
  BlockedInfoRowDataForSelect,
  BlockedListStatic,
  BlockedInfoRowText,
  dummyData
} from 'brave-ui/features/shields'

// Group components
import StaticResourcesList from '../../shared/resourcesBlockedList/staticResourcesList'

// Helpers
import { getLocale } from '../../../background/api/localeAPI'

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
        <BlockedInfoRowForSelectSummary onClick={this.onClickFakeThirdPartyFingerprintingBlocked}>
          <BlockedInfoRowDataForSelect disabled={false}>
            {
              dummyThirdPartyFingerprintingBlockedOpen
                ? <ArrowUpIcon />
                : <ArrowDownIcon />
            }
            <BlockedInfoRowStats>{2}</BlockedInfoRowStats>
            <BlockedInfoRowText>
              <span>{getLocale('thirdPartyFingerprintingBlocked')}</span>
            </BlockedInfoRowText>
          </BlockedInfoRowDataForSelect>
        </BlockedInfoRowForSelectSummary>
        <BlockedListStatic>
          <StaticResourcesList list={dummyData.otherBlockedResources} />
        </BlockedListStatic>
      </BlockedInfoRowDetails>
    )
  }
}
