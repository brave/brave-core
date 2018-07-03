/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Grid, Column } from 'brave-ui/gridSystem'
import TextLabel from 'brave-ui/textLabel'
import Paragraph from 'brave-ui/paragraph'

import { BlockOptions } from '../../types/other/blockTypes'
import { getMessage } from '../../background/api/localeAPI'
import theme from '../../theme'

export interface BraveShieldsStatsProps {
  braveShields: BlockOptions
  adsBlocked: number
  trackersBlocked: number
  httpsRedirected: number
  javascriptBlocked: number
  fingerprintingBlocked: number
  adsBlockedResources: Array<string>
  trackersBlockedResources: Array<string>
  httpsRedirectedResources: Array<string>
  javascriptBlockedResources: Array<string>
  fingerprintingBlockedResources: Array<string>
}

export interface BraveShieldsStatsState {
  adsTrackersBlockedDetailsOpen: boolean,
  httpsRedirectedDetailsOpen: boolean,
  javascriptBlockedDetailsOpen: boolean,
  fingerprintingBlockedDetailsOpen: boolean
}

export default class BraveShieldsStats extends React.Component<BraveShieldsStatsProps, BraveShieldsStatsState> {
  constructor (props: BraveShieldsStatsProps) {
    super(props)
    this.onToggleBlockedResourcesDetails = this.onToggleBlockedResourcesDetails.bind(this)

    // creates a temporary state to switch the toggle of blocked resources details
    this.state = {
      adsTrackersBlockedDetailsOpen: false,
      httpsRedirectedDetailsOpen: false,
      javascriptBlockedDetailsOpen: false,
      fingerprintingBlockedDetailsOpen: false
    }
  }

  get totalAdsTrackersBlocked (): number {
    const { adsBlocked, trackersBlocked } = this.props
    return adsBlocked + trackersBlocked
  }

  get httpsRedirected (): number {
    return this.props.httpsRedirected
  }

  get javascriptBlocked (): number {
    return this.props.javascriptBlocked
  }

  get fingerprintingBlocked (): number {
    return this.props.fingerprintingBlocked
  }

  get totalAdsTrackersBlockedResources (): Array<string> {
    const { adsBlockedResources, trackersBlockedResources } = this.props
    return [ ...adsBlockedResources, ...trackersBlockedResources ]
  }

  get httpsRedirectedResources (): Array<string> {
    return this.props.httpsRedirectedResources
  }

  get javascriptBlockedResources (): Array<string> {
    return this.props.javascriptBlockedResources
  }

  get fingerprintingBlockedResources (): Array<string> {
    return this.props.fingerprintingBlockedResources
  }

  get blockedResourcesDetailsList () {
    const {
      adsTrackersBlockedDetailsOpen,
      httpsRedirectedDetailsOpen,
      javascriptBlockedDetailsOpen,
      fingerprintingBlockedDetailsOpen
    } = this.state

    if (adsTrackersBlockedDetailsOpen && this.totalAdsTrackersBlocked > 0) {
      return this.setResourceBlockedItemView(this.totalAdsTrackersBlockedResources)
    } else if (httpsRedirectedDetailsOpen && this.httpsRedirected > 0) {
      return this.setResourceBlockedItemView(this.httpsRedirectedResources)
    } else if (javascriptBlockedDetailsOpen && this.javascriptBlocked > 0) {
      return this.setResourceBlockedItemView(this.javascriptBlockedResources)
    } else if (fingerprintingBlockedDetailsOpen && this.fingerprintingBlocked > 0) {
      return this.setResourceBlockedItemView(this.fingerprintingBlockedResources)
    } else {
      return null
    }
  }

  setResourceBlockedItemView (resource: Array<string>) {
    return resource.map((src: string, key: number) =>
      <Paragraph theme={theme.blockedResourcesStatsText} key={key} text={src} />)
  }

  onToggleBlockedResourcesDetails (e: any) {
    const id = e.target.id || ''
    const {
      adsTrackersBlockedDetailsOpen,
      httpsRedirectedDetailsOpen,
      javascriptBlockedDetailsOpen,
      fingerprintingBlockedDetailsOpen
    } = this.state

    this.setState({
      adsTrackersBlockedDetailsOpen: id.startsWith('totalAdsTrackersBlocked') && !adsTrackersBlockedDetailsOpen,
      httpsRedirectedDetailsOpen: id.startsWith('httpsRedirected') && !httpsRedirectedDetailsOpen,
      javascriptBlockedDetailsOpen: id.startsWith('javascriptBlocked') && !javascriptBlockedDetailsOpen,
      fingerprintingBlockedDetailsOpen: id.startsWith('fingerprintingBlocked') && !fingerprintingBlockedDetailsOpen
    })
  }

  render () {
    const { braveShields } = this.props
    return (
      <Grid
        id='braveShieldsStats'
        theme={theme.braveShieldsStats}
        disabled={braveShields === 'block'}
      >
        <Column theme={theme.statsNumbers} size={1}>
          <TextLabel
            id='totalAdsTrackersBlockedStat'
            theme={theme.totalAdsTrackersBlockedStat}
            text={this.totalAdsTrackersBlocked}
            onClick={this.onToggleBlockedResourcesDetails}
          />
          <TextLabel
            id='httpsRedirectedStat'
            theme={theme.httpsRedirectedStat}
            text={this.httpsRedirected}
            onClick={this.onToggleBlockedResourcesDetails}
          />
          <TextLabel
            id='javascriptBlockedStat'
            theme={theme.javascriptBlockedStat}
            text={this.javascriptBlocked}
            onClick={this.onToggleBlockedResourcesDetails}
          />
          <TextLabel
            id='fingerprintingBlockedStat'
            theme={theme.fingerprintingBlockedStat}
            text={this.fingerprintingBlocked}
            onClick={this.onToggleBlockedResourcesDetails}
          />
        </Column>
        <Column theme={theme.statsNames} size={11}>
          <TextLabel
            id='totalAdsTrackersBlockedText'
            theme={theme.totalAdsTrackersBlockedText}
            text={getMessage('shieldsStatsAdsTrackersBlocked')}
            onClick={this.onToggleBlockedResourcesDetails}
          />
          <TextLabel
            id='httpsRedirectedText'
            theme={theme.httpsRedirectedText}
            text={getMessage('shieldsStatsHttpsUpgrades')}
            onClick={this.onToggleBlockedResourcesDetails}
          />
          <TextLabel
            id='javascriptBlockedText'
            theme={theme.javascriptBlockedText}
            text={getMessage('shieldsStatsScriptsBlocked')}
            onClick={this.onToggleBlockedResourcesDetails}
          />
          <TextLabel
            id='fingerprintingBlockedText'
            theme={theme.fingerprintingBlockedText}
            text={getMessage('shieldsFingerPrintingBlocked')}
            onClick={this.onToggleBlockedResourcesDetails}
          />
        </Column>
        <Column id='blockedResourcesStats' theme={theme.blockedResourcesStats} size={12}>
          {this.blockedResourcesDetailsList}
        </Column>
      </Grid>
    )
  }
}
