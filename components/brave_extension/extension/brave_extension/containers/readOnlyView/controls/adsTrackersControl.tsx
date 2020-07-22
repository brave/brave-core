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
  sumAdsAndTrackers,
  blockedResourcesSize,
  shouldDisableResourcesRow,
  mergeAdsAndTrackersResources
} from '../../../helpers/shieldsUtils'

// Types
import { BlockOptions } from '../../../types/other/blockTypes'

interface Props {
  ads: BlockOptions
  adsBlocked: number
  adsBlockedResources: Array<string>
  trackers: BlockOptions
  trackersBlocked: number
  trackersBlockedResources: Array<string>
}

interface State {
  trackersBlockedOpen: boolean
}

export default class AdsTrackersControl extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = { trackersBlockedOpen: false }
  }

  get totalAdsTrackersBlocked (): number {
    const { adsBlocked, trackersBlocked } = this.props
    return sumAdsAndTrackers(adsBlocked, trackersBlocked)
  }

  get totalAdsTrackersBlockedDisplay (): string {
    return blockedResourcesSize(this.totalAdsTrackersBlocked)
  }

  get totalAdsTrackersBlockedList (): Array<string> {
    const { adsBlockedResources, trackersBlockedResources } = this.props
    return mergeAdsAndTrackersResources(adsBlockedResources, trackersBlockedResources)
  }

  get shouldDisableResourcesRow (): boolean {
    return shouldDisableResourcesRow(this.totalAdsTrackersBlocked)
  }

  triggerOpen3rdPartyTrackersBlocked = () => {
    if (!this.shouldDisableResourcesRow) {
      this.setState({ trackersBlockedOpen: !this.state.trackersBlockedOpen })
    }
  }

  render () {
    const { trackersBlockedOpen } = this.state
    return (
      <BlockedInfoRowDetails>
        <BlockedInfoRowSummary onClick={this.triggerOpen3rdPartyTrackersBlocked}>
          <BlockedInfoRowData disabled={this.shouldDisableResourcesRow}>
            {
              trackersBlockedOpen
                ? <ArrowUpIcon />
                : <ArrowDownIcon />
            }
            <BlockedInfoRowStats>{this.totalAdsTrackersBlockedDisplay}</BlockedInfoRowStats>
            <BlockedInfoRowText>{getLocale('readOnlyAdsTrackersBlocking')}</BlockedInfoRowText>
          </BlockedInfoRowData>
        </BlockedInfoRowSummary>
        <BlockedListStatic>
          <StaticResourcesList list={this.totalAdsTrackersBlockedList} />
        </BlockedListStatic>
      </BlockedInfoRowDetails>
    )
  }
}
