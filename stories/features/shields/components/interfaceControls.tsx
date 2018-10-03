/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Toggle, Stat, GridLabel } from '../../../../src/features/shields'
import locale from '../fakeLocale'
import data from '../fakeData'

interface Props {
  enabled: boolean
}

// Fake stuff
interface State {
  blockAds: boolean
  blockPopups: boolean
  blockImages: boolean
}

export default class InterfaceControls extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      blockAds: false,
      blockPopups: false,
      blockImages: false
    }
  }

  onChangeBlockAds = (event: React.ChangeEvent<HTMLInputElement>) => {
    this.setState({ blockAds: event.target.checked })
  }

  onChangeBlockPopups = (event: React.ChangeEvent<HTMLInputElement>) => {
    this.setState({ blockPopups: event.target.checked })
  }

  onChangeBlockImages = (event: React.ChangeEvent<HTMLInputElement>) => {
    this.setState({ blockImages: event.target.checked })
  }

  render () {
    const { enabled } = this.props
    const { blockAds, blockPopups, blockImages } = this.state

    if (!enabled) {
      return null
    }

    return (
      <>
        {/* ads toggle */}
          <GridLabel>
            <Stat>{data.totalAdsTrackersBlocked}</Stat>
            <span>{locale.blockAds}</span>
            <Toggle id='blockAds' checked={blockAds} onChange={this.onChangeBlockAds} />
          </GridLabel>
        {/* popups toggle */}
          <GridLabel>
            <Stat>{data.popupsBlocked}</Stat>
            <span>{locale.blockPopups}</span>
            <Toggle id='blockPopups' checked={blockPopups} onChange={this.onChangeBlockPopups} />
          </GridLabel>
        {/* image toggle */}
          <GridLabel>
            <Stat>{data.imagesBlocked}</Stat>
            <span>{locale.blockImages}</span>
            <Toggle id='blockImages' checked={blockImages} onChange={this.onChangeBlockImages} />
          </GridLabel>
      </>
    )
  }
}
