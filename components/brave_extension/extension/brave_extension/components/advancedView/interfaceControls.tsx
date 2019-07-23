/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Group Components
import AdsTrackersControl from './controls/adsTrackersControl'
import HTTPSUpgradesControl from './controls/httpsUpgradesControl'

// Types
import { BlockAdsTrackers, HttpsEverywhereToggled } from '../../types/actions/shieldsPanelActions'
import { BlockOptions } from '../../types/other/blockTypes'

interface CommonProps {
  isBlockedListOpen: boolean
  setBlockedListOpen: () => void
  hostname: string
  favicon: string
}

interface AdsTrackersProps {
  ads: BlockOptions
  adsBlocked: number
  adsBlockedResources: Array<string>
  trackers: BlockOptions
  trackersBlocked: number
  trackersBlockedResources: Array<string>
  blockAdsTrackers: BlockAdsTrackers
}

interface HTTPSUpgradesProps {
  httpsRedirected: number
  httpUpgradableResources: BlockOptions
  httpsRedirectedResources: Array<string>
  httpsEverywhereToggled: HttpsEverywhereToggled
}

export type Props = CommonProps & AdsTrackersProps & HTTPSUpgradesProps

export default class InterfaceControls extends React.PureComponent<Props, {}> {
  get commonProps (): CommonProps {
    const { favicon, hostname, isBlockedListOpen, setBlockedListOpen } = this.props
    return { favicon, hostname, isBlockedListOpen, setBlockedListOpen }
  }

  render () {
    return (
      <>
        <AdsTrackersControl
          {...this.commonProps}
          ads={this.props.ads}
          adsBlocked={this.props.adsBlocked}
          adsBlockedResources={this.props.adsBlockedResources}
          trackers={this.props.trackers}
          trackersBlocked={this.props.trackersBlocked}
          trackersBlockedResources={this.props.trackersBlockedResources}
          blockAdsTrackers={this.props.blockAdsTrackers}
        />
        <HTTPSUpgradesControl
          {...this.commonProps}
          httpsRedirected={this.props.httpsRedirected}
          httpUpgradableResources={this.props.httpUpgradableResources}
          httpsRedirectedResources={this.props.httpsRedirectedResources}
          httpsEverywhereToggled={this.props.httpsEverywhereToggled}
        />
      </>
    )
  }
}
