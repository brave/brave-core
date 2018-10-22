/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  ShowMoreIcon,
  Toggle,
  Stat,
  GridLabel,
  ResourcesLabel,
  EmptyButton
} from '../../../../src/features/shields'

// Component groups
import BlockedResources from './blockedReources/blockedResources'
import StaticList from './blockedReources/staticList'

// Fake data
import locale from '../fakeLocale'
import data from '../fakeData'

interface Props {
  enabled: boolean
  sitename: string
  favicon: string
}

interface State {
  blockAds: boolean
  openTotalAdsTrackersBlockedList: boolean
}

export default class InterfaceControls extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      blockAds: false,
      openTotalAdsTrackersBlockedList: false
    }
  }

  get totalAdsTrackersBlockedList () {
    const { favicon, sitename } = this.props
    return (
      <BlockedResources
        favicon={favicon}
        sitename={sitename}
        title={locale.blockAds}
        onToggle={this.onToggleTotalAdsTrackersBlocked}
        data={data.totalAdsTrackersBlocked}
      >
        <StaticList
          onClickDismiss={this.onToggleTotalAdsTrackersBlocked}
          list={data.blockedFakeResources}
        />
      </BlockedResources>
    )
  }

  onChangeBlockAds = (event: React.ChangeEvent<HTMLInputElement>) => {
    this.setState({ blockAds: event.target.checked })
  }

  // onChangeBlockPopups = (event: React.ChangeEvent<HTMLInputElement>) => {
  //   this.setState({ blockPopups: event.target.checked })
  // }

  // onChangeBlockImages = (event: React.ChangeEvent<HTMLInputElement>) => {
  //   this.setState({ blockImages: event.target.checked })
  // }

  onToggleTotalAdsTrackersBlocked = () => {
    this.setState({ openTotalAdsTrackersBlockedList: !this.state.openTotalAdsTrackersBlockedList })
  }

  render () {
    const { enabled } = this.props
    const { blockAds, openTotalAdsTrackersBlockedList /* blockPopups, blockImages */ } = this.state

    if (!enabled) {
      return null
    }

    return (
      <>
        {/* ads toggle */}
          <GridLabel>
            <EmptyButton onClick={this.onToggleTotalAdsTrackersBlocked}><ShowMoreIcon /></EmptyButton>
            <Stat>{data.totalAdsTrackersBlocked}</Stat>
            <ResourcesLabel>{locale.blockAds}</ResourcesLabel>
            <Toggle id='blockAds' checked={blockAds} onChange={this.onChangeBlockAds} />
          </GridLabel>
          {openTotalAdsTrackersBlockedList ? this.totalAdsTrackersBlockedList : null}
        {/* popups toggle
          <GridLabel>
            <Stat>{data.popupsBlocked}</Stat>
            <span>{locale.blockPopups}</span>
            <Toggle id='blockPopups' checked={blockPopups} onChange={this.onChangeBlockPopups} />
          </GridLabel> */}
        {/* image toggle
          <GridLabel>
            <Stat>{data.imagesBlocked}</Stat>
            <span>{locale.blockImages}</span>
            <Toggle id='blockImages' checked={blockImages} onChange={this.onChangeBlockImages} />
          </GridLabel> */}
      </>
    )
  }
}
