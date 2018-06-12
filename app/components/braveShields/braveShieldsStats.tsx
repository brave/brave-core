/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Grid, Column } from 'brave-ui/gridSystem'
import TextLabel from 'brave-ui/textLabel'

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
}

export default class BraveShieldsStats extends React.Component<BraveShieldsStatsProps, {}> {
  constructor (props: BraveShieldsStatsProps) {
    super(props)
    this.onClickadsTrackersBlockedStats = this.onClickadsTrackersBlockedStats.bind(this)
    this.onClickHttpsUpgradesStats = this.onClickHttpsUpgradesStats.bind(this)
    this.onClickScriptsBlockedStats = this.onClickScriptsBlockedStats.bind(this)
    this.onClickFingerPrintingProtectionStats = this.onClickScriptsBlockedStats.bind(this)
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

  onClickadsTrackersBlockedStats () {
    // TODO #202
    console.log('fired adsTrackersBlockedStats')
  }

  onClickHttpsUpgradesStats () {
    // TODO #202
    console.log('fired httpsUpgradesStats')
  }

  onClickScriptsBlockedStats () {
    // TODO #202
    console.log('fired scriptsBlockedStats')
  }

  onClickFingerPrintingProtectionStats () {
    // TODO #202
    console.log('fired fingerPrintingProtectionStats')
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
            theme={theme.totalAdsTrackersBlockedStat}
            text={this.totalAdsTrackersBlocked}
            onClick={this.onClickadsTrackersBlockedStats}
          />
          <TextLabel
            theme={theme.httpsRedirectedStat}
            text={this.httpsRedirected}
            onClick={this.onClickHttpsUpgradesStats}
          />
          <TextLabel
            theme={theme.javascriptBlockedStat}
            text={this.javascriptBlocked}
            onClick={this.onClickScriptsBlockedStats}
          />
          <TextLabel
            theme={theme.fingerprintingBlockedStat}
            text={this.fingerprintingBlocked}
            onClick={this.onClickFingerPrintingProtectionStats}
          />
        </Column>
        <Column theme={theme.statsNames} size={11}>
          <TextLabel
            theme={theme.totalAdsTrackersBlockedText}
            text={getMessage('shieldsStatsAdsTrackersBlocked')}
            onClick={this.onClickadsTrackersBlockedStats}
          />
          <TextLabel
            theme={theme.httpsRedirectedText}
            text={getMessage('shieldsStatsHttpsUpgrades')}
            onClick={this.onClickHttpsUpgradesStats}
          />
          <TextLabel
            theme={theme.javascriptBlockedText}
            text={getMessage('shieldsStatsScriptsBlocked')}
            onClick={this.onClickScriptsBlockedStats}
          />
          <TextLabel
            theme={theme.fingerprintingBlockedText}
            text={getMessage('shieldsFingerPrintingBlocked')}
            onClick={this.onClickFingerPrintingProtectionStats}
          />
        </Column>
      </Grid>
    )
  }
}
