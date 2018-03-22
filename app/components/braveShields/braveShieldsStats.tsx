/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Grid, Column, BrowserText } from 'brave-ui'
import { BlockOptions } from '../../types/other/blockTypes'
import { getMessage } from '../../background/api/localeAPI'

export interface Props {
  braveShields: BlockOptions
  adsBlocked: number
  trackersBlocked: number
  httpsRedirected: number
  javascriptBlocked: number
  fingerprintingBlocked: number
}

export default class BraveShieldsStats extends React.Component<Props, object> {
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

  render () {
    const { braveShields } = this.props
    return (
      <Grid
        id='braveShieldsStats'
        disabled={braveShields === 'block'}
        padding='10px'
        gap='7px'
        background='#f7f7f7'
      >
        <Column align='flex-end' size={2}>
          <BrowserText
            noSelect={true}
            text={this.totalAdsTrackersBlocked}
            fontSize='26px'
            color='#fe521d'
          />
        </Column>
        <Column
          id='adsTrackersBlockedStats'
          size={10}
          verticalAlign='center'
        >
          <BrowserText text={getMessage('shieldsStatsAdsTrackersBlocked')} />
        </Column>

        <Column align='flex-end' size={2}>
          <BrowserText noSelect={true} text={this.httpsRedirected} fontSize='26px' color='#0796fa' />
        </Column>
        <Column
          id='httpsUpgradesStats'
          size={10}
          verticalAlign='center'
        >
          <BrowserText text={getMessage('shieldsStatsHttpsUpgrades')} />
        </Column>

        <Column align='flex-end' size={2}>
          <BrowserText noSelect={true} text={this.javascriptBlocked} fontSize='26px' color='#555555' />
        </Column>
        <Column
          id='scriptsBlockedStats'
          size={10}
          verticalAlign='center'
        >
          <BrowserText text={getMessage('shieldsStatsScriptsBlocked')} />
        </Column>

        <Column align='flex-end' size={2}>
          <BrowserText noSelect={true} text={this.fingerprintingBlocked} fontSize='26px' color='#ffc000' />
        </Column>
        <Column
          id='fingerPrintingProtectionStats'
          size={10}
          verticalAlign='center'
        >
          <BrowserText text={getMessage('shieldsFingerPrintingBlocked')} />
        </Column>
      </Grid>
    )
  }
}
