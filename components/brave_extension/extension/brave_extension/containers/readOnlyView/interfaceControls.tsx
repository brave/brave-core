/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Group components
import AdsTrackersControl from './controls/adsTrackersControl'
import HTTPSUpgradesControl from './controls/httpsUpgradesControl'

// Types
import { BlockOptions } from '../../types/other/blockTypes'

interface AdsTrackersProps {
  ads: BlockOptions
  adsBlocked: number
  adsBlockedResources: Array<string>
  trackers: BlockOptions
  trackersBlocked: number
  trackersBlockedResources: Array<string>
}

interface HTTPSUpgradesProps {
  httpsRedirected: number
  httpUpgradableResources: BlockOptions
  httpsRedirectedResources: Array<string>
}

export type Props = AdsTrackersProps & HTTPSUpgradesProps

export default class InterfaceControls extends React.PureComponent<Props, {}> {
  render () {
    return (
      <>
        <AdsTrackersControl
          ads={this.props.ads}
          adsBlocked={this.props.adsBlocked}
          adsBlockedResources={this.props.adsBlockedResources}
          trackers={this.props.trackers}
          trackersBlocked={this.props.trackersBlocked}
          trackersBlockedResources={this.props.trackersBlockedResources}
        />
        <HTTPSUpgradesControl
          httpsRedirected={this.props.httpsRedirected}
          httpUpgradableResources={this.props.httpUpgradableResources}
          httpsRedirectedResources={this.props.httpsRedirectedResources}
        />
      </>
    )
  }
}
