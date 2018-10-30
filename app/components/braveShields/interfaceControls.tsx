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
} from 'brave-ui/features/shields'

// Component groups
import BlockedResources from './blockedResources/blockedResources'
import StaticList from './blockedResources/staticList'

// Types
import { BlockOptions } from '../../types/other/blockTypes'

// Utils
import { getMessage } from '../../background/api/localeAPI'
import * as shieldActions from '../../types/actions/shieldsPanelActions'
import { totalAdsTrackersBlocked } from '../../helpers/shieldsUtils'

export interface Props {
  braveShields: BlockOptions
  url: string
  hostname: string
  ads: BlockOptions
  adsBlocked: number
  adsBlockedResources: Array<string>
  blockAdsTrackers: shieldActions.BlockAdsTrackers
  trackers: BlockOptions
  trackersBlocked: number
  trackersBlockedResources: Array<string>
  httpsRedirected: number
  httpUpgradableResources: BlockOptions
  httpsRedirectedResources: Array<string>
  httpsEverywhereToggled: shieldActions.HttpsEverywhereToggled
}

interface State {
  openTotalAdsTrackersBlockedList: boolean,
  openConnectionsEncryptedList: boolean
}

export default class ShieldsInterfaceControls extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      openTotalAdsTrackersBlockedList: false,
      openConnectionsEncryptedList: false
    }
  }

  get totalAdsTrackersBlocked (): number {
    const { adsBlocked, trackersBlocked } = this.props
    return totalAdsTrackersBlocked(adsBlocked, trackersBlocked)
  }

  get totalAdsTrackersBlockedList (): Array<string> {
    const { adsBlockedResources, trackersBlockedResources } = this.props
    return [...adsBlockedResources, ...trackersBlockedResources]
  }

  onChangeBlockAds = (event: React.ChangeEvent<HTMLInputElement>) => {
    if (!event.target) {
      return
    }
    const shoudEnableAdsTracks = event.target.checked ? 'allow' : 'block'
    this.props.blockAdsTrackers(shoudEnableAdsTracks)
  }

  onChangeConnectionsEncrypted = (event: React.ChangeEvent<HTMLInputElement>) => {
    if (!event.target) {
      return
    }
    const shouldEnableHttpsEverywhere = event.target.checked ? 'allow' : 'block'
    this.props.httpsEverywhereToggled(shouldEnableHttpsEverywhere)
  }

  onToggleTotalAdsTrackersBlocked = () => {
    this.setState({ openTotalAdsTrackersBlockedList: !this.state.openTotalAdsTrackersBlockedList })
  }

  onToggleConnectionsEncrypted = () => {
    this.setState({ openConnectionsEncryptedList: !this.state.openConnectionsEncryptedList })
  }

  renderTotalAdsTrackersBlockedList = (url: string, hostname: string, onToggle: () => void) => {
    return (
      <BlockedResources
        url={url}
        hostname={hostname}
        title={getMessage('blockAds')}
        onToggle={onToggle}
        stats={this.totalAdsTrackersBlocked}
      >
        <StaticList onClickDismiss={onToggle} list={this.totalAdsTrackersBlockedList} />
      </BlockedResources>
    )
  }

  renderConnectionsEncryptedList = (url: string, hostname: string, onToggle: () => void) => {
    const { httpsRedirected, httpsRedirectedResources } = this.props
    return (
      <BlockedResources
        url={url}
        hostname={hostname}
        title={getMessage('connectionsEncrypted')}
        onToggle={onToggle}
        stats={httpsRedirected}
      >
        <StaticList onClickDismiss={onToggle} list={httpsRedirectedResources} />
      </BlockedResources>
    )
  }

  render () {
    const { braveShields, url, hostname, ads, trackers, httpsRedirected, httpUpgradableResources } = this.props
    const { openTotalAdsTrackersBlockedList, openConnectionsEncryptedList } = this.state

    const enabled = braveShields !== 'block'
    const blockAdsOption = ads !== 'allow' && trackers !== 'allow'
    const connectionsEncrypted = httpUpgradableResources !== 'allow'

    if (!enabled) {
      return null
    }

    return (
      <div id='braveShieldsInterfaceControls'>
        {/* ads toggle */}
        <ToggleGrid>
          <EmptyButton disabled={this.totalAdsTrackersBlocked === 0} onClick={this.onToggleTotalAdsTrackersBlocked}><ShowMoreIcon /></EmptyButton>
          <StatFlex id='blockAdsStat' onClick={this.onToggleTotalAdsTrackersBlocked}>{this.totalAdsTrackersBlocked}</StatFlex>
          <ResourcesSwitchLabel onClick={this.onToggleTotalAdsTrackersBlocked}>{getMessage('blockAds')}</ResourcesSwitchLabel>
          <ToggleFlex><Toggle id='blockAds' checked={blockAdsOption} onChange={this.onChangeBlockAds} /></ToggleFlex>
        </ToggleGrid>
        {
          openTotalAdsTrackersBlockedList
            ? this.renderTotalAdsTrackersBlockedList(url, hostname, this.onToggleTotalAdsTrackersBlocked)
            : null
        }
        {/* connections encrypted toggle */}
        <ToggleGrid>
          <EmptyButton disabled={httpsRedirected === 0} onClick={this.onToggleConnectionsEncrypted}><ShowMoreIcon /></EmptyButton>
          <StatFlex id='connectionsEncryptedStat' onClick={this.onToggleConnectionsEncrypted}>{httpsRedirected}</StatFlex>
          <ResourcesSwitchLabel onClick={this.onToggleConnectionsEncrypted}>{getMessage('connectionsEncrypted')}</ResourcesSwitchLabel>
          <ToggleFlex><Toggle id='connectionsEncrypted' checked={connectionsEncrypted} onChange={this.onChangeConnectionsEncrypted} /></ToggleFlex>
        </ToggleGrid>
        {
          openConnectionsEncryptedList
            ? this.renderConnectionsEncryptedList(url, hostname, this.onToggleConnectionsEncrypted)
            : null
        }
      </div>
    )
  }
}
