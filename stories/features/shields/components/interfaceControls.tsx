/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  ShowMoreIcon,
  Toggle,
  Stat,
  ResourcesSwitchLabel,
  EmptyButton,
  ToggleFlex,
  ToggleGrid
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
  connectionsEncrypted: boolean
  openTotalAdsTrackersBlockedList: boolean
  openConnectionsEncryptedList: boolean
}

export default class InterfaceControls extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      blockAds: false,
      openTotalAdsTrackersBlockedList: false,
      connectionsEncrypted: false,
      openConnectionsEncryptedList: false
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

  get connectionsEncryptedList () {
    const { favicon, sitename } = this.props
    return (
      <BlockedResources
        favicon={favicon}
        sitename={sitename}
        title={locale.connectionsEncrypted}
        onToggle={this.onToggleConnectionsEncrypted}
        data={data.thirdPartyDeviceRecognitionBlocked}
      >
        <StaticList
          onClickDismiss={this.onToggleConnectionsEncrypted}
          list={data.blockedFakeResources}
        />
      </BlockedResources>
    )
  }

  onChangeBlockAds = (event: React.ChangeEvent<HTMLInputElement>) => {
    this.setState({ blockAds: event.target.checked })
  }

  onChangeConnectionsEncrypted = (event: React.ChangeEvent<HTMLInputElement>) => {
    this.setState({ connectionsEncrypted: event.target.checked })
  }

  onToggleTotalAdsTrackersBlocked = () => {
    this.setState({ openTotalAdsTrackersBlockedList: !this.state.openTotalAdsTrackersBlockedList })
  }

  onToggleConnectionsEncrypted = () => {
    this.setState({ openConnectionsEncryptedList: !this.state.openConnectionsEncryptedList })
  }

  // onChangeBlockPopups = (event: React.ChangeEvent<HTMLInputElement>) => {
  //   this.setState({ blockPopups: event.target.checked })
  // }

  // onChangeBlockImages = (event: React.ChangeEvent<HTMLInputElement>) => {
  //   this.setState({ blockImages: event.target.checked })
  // }

  render () {
    const { enabled } = this.props
    const {
      blockAds,
      openTotalAdsTrackersBlockedList,
      connectionsEncrypted,
      openConnectionsEncryptedList /* blockPopups, blockImages */
    } = this.state

    if (!enabled) {
      return null
    }

    return (
      <>
        {/* ads toggle */}
          <ToggleGrid>
            <EmptyButton onClick={this.onToggleTotalAdsTrackersBlocked}><ShowMoreIcon /></EmptyButton>
            <Stat onClick={this.onToggleTotalAdsTrackersBlocked}>{data.totalAdsTrackersBlocked}</Stat>
            <ResourcesSwitchLabel onClick={this.onToggleTotalAdsTrackersBlocked}>{locale.blockAds}</ResourcesSwitchLabel>
            <ToggleFlex><Toggle id='blockAds' checked={blockAds} onChange={this.onChangeBlockAds} /></ToggleFlex>
          </ToggleGrid>
          {openTotalAdsTrackersBlockedList ? this.totalAdsTrackersBlockedList : null}
        {/* connections encrypted toggle */}
        <ToggleGrid>
          <EmptyButton onClick={this.onToggleConnectionsEncrypted}><ShowMoreIcon /></EmptyButton>
          <Stat onClick={this.onToggleConnectionsEncrypted}>{data.connectionsEncrypted}</Stat>
          <ResourcesSwitchLabel onClick={this.onToggleConnectionsEncrypted}>{locale.connectionsEncrypted}</ResourcesSwitchLabel>
          <ToggleFlex><Toggle id='connectionsEncrypted' checked={connectionsEncrypted} onChange={this.onChangeConnectionsEncrypted} /></ToggleFlex>
        </ToggleGrid>
        {openConnectionsEncryptedList ? this.connectionsEncryptedList : null}
        {/* popups toggle
          <ToggleGrid>
            <Stat>{data.popupsBlocked}</Stat>
            <span>{locale.blockPopups}</span>
            <Toggle id='blockPopups' checked={blockPopups} onChange={this.onChangeBlockPopups} />
          </ToggleGrid> */}
        {/* image toggle
          <ToggleGrid>
            <Stat>{data.imagesBlocked}</Stat>
            <span>{locale.blockImages}</span>
            <Toggle id='blockImages' checked={blockImages} onChange={this.onChangeBlockImages} />
          </ToggleGrid> */}
      </>
    )
  }
}
