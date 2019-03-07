/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// Feature-specific components
import {
  ShowMoreIcon,
  Toggle,
  StatFlex,
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

const totalAdsTrackersBlockedList = (favicon: string, sitename: string, onToggle: () => void) => {
  return (
    <BlockedResources
      favicon={favicon}
      sitename={sitename}
      title={locale.blockAds}
      onToggle={onToggle}
      data={data.totalAdsTrackersBlocked}
    >
      <StaticList onClickDismiss={onToggle} list={data.blockedFakeResources} />
    </BlockedResources>
  )
}

const connectionsEncryptedList = (favicon: string, sitename: string, onToggle: () => void) => {
  return (
    <BlockedResources
      favicon={favicon}
      sitename={sitename}
      title={locale.connectionsEncrypted}
      onToggle={onToggle}
      data={data.thirdPartyDeviceRecognitionBlocked}
    >
      <StaticList onClickDismiss={onToggle} list={data.blockedFakeResources} />
    </BlockedResources>
  )
}

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

  render () {
    const { enabled, favicon, sitename } = this.props
    const {
      blockAds,
      openTotalAdsTrackersBlockedList,
      connectionsEncrypted,
      openConnectionsEncryptedList
    } = this.state

    if (!enabled) {
      return null
    }

    return (
      <>
        {/* ads toggle */}
          <ToggleGrid>
            <EmptyButton onClick={this.onToggleTotalAdsTrackersBlocked}><ShowMoreIcon /></EmptyButton>
            <StatFlex onClick={this.onToggleTotalAdsTrackersBlocked}>{data.totalAdsTrackersBlocked}</StatFlex>
            <ResourcesSwitchLabel onClick={this.onToggleTotalAdsTrackersBlocked}>{locale.blockAds}</ResourcesSwitchLabel>
            <ToggleFlex><Toggle id='blockAds' checked={blockAds} onChange={this.onChangeBlockAds} /></ToggleFlex>
          </ToggleGrid>
          {openTotalAdsTrackersBlockedList ? totalAdsTrackersBlockedList(favicon, sitename, this.onToggleTotalAdsTrackersBlocked) : null}
        {/* connections encrypted toggle */}
        <ToggleGrid>
          <EmptyButton disabled={true} onClick={this.onToggleConnectionsEncrypted}><ShowMoreIcon /></EmptyButton>
          <StatFlex disabled={true} onClick={this.onToggleConnectionsEncrypted}>0</StatFlex>
          <ResourcesSwitchLabel disabled={true} onClick={this.onToggleConnectionsEncrypted}>{locale.connectionsEncrypted}</ResourcesSwitchLabel>
          <ToggleFlex><Toggle id='connectionsEncrypted' checked={connectionsEncrypted} onChange={this.onChangeConnectionsEncrypted} /></ToggleFlex>
        </ToggleGrid>
        {openConnectionsEncryptedList ? connectionsEncryptedList(favicon, sitename, this.onToggleConnectionsEncrypted) : null}
      </>
    )
  }
}
