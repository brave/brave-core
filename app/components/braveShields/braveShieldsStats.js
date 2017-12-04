/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import React, { Component } from 'react'
import { Grid, Column, BrowserText } from 'brave-ui'
export default class BraveShieldsStats extends Component {
  get totalAdsTrackersBlocked () {
    const { adsBlocked, trackingProtectionBlocked } = this.props
    return adsBlocked + trackingProtectionBlocked
  }
  get httpsEverywhereRedirected () {
    return this.props.httpsEverywhereRedirected
  }
  render () {
    const { shieldsEnabled } = this.props
    return (
      <Grid
        id='braveShieldsStats'
        disabled={shieldsEnabled === 'block'}
        padding='10px'
        gap='7px'
        background='#f7f7f7'
      >
        <Column align='flex-end' size={2}>
          <BrowserText
            noSelect
            text={this.totalAdsTrackersBlocked}
            fontSize='26px'
            color='#fe521d'
          />
        </Column>
        <Column
          id='adsTrackersBlockedStats'
          size={10}
          verticalAlign='center'>
          Ads and Trackers Blocked
        </Column>

        <Column align='flex-end' size={2}>
          <BrowserText noSelect text={this.httpsEverywhereRedirected} fontSize='26px' color='#0796fa' />
        </Column>
        <Column
          id='httpsUpgradesStats'
          size={10}
          verticalAlign='center'>
          HTTPS Upgrades
        </Column>

        <Column align='flex-end' size={2}>
          <BrowserText noSelect text='0' fontSize='26px' color='#555555' />
        </Column>
        <Column
          id='scriptsBlockedStats'
          size={10}
          verticalAlign='center'>
          Scripts Blocked
        </Column>

        <Column align='flex-end' size={2}>
          <BrowserText noSelect text='0' fontSize='26px' color='#ffc000' />
        </Column>
        <Column
          id='fingerPrintingProtectionStats'
          size={10}
          verticalAlign='center'>
            Fingerprinting Methods Blocked
        </Column>
      </Grid>
    )
  }
}
