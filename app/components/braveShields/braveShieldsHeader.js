/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import React, { Component } from 'react'
import { Grid, Column, Separator, SwitchButton, ActionButton, BrowserText } from 'brave-ui'

export default class BraveShieldsHeader extends Component {
  constructor () {
    super()
    this.onClosePopup = this.onClosePopup.bind(this)
  }
  onClosePopup () {
    window.close()
  }
  render () {
    const { shieldsToggled, hostname } = this.props
    return (
      <div>
        <Grid
          id='shieldsHeader'
          background='#808080'
          padding='10px'
          gap='0'
          textColor='#fafafa'
        >
          <Column size={4} verticalAlign='center'>
            <BrowserText noSelect fontSize='14px' text='Shields' />
          </Column>
          <Column size={6} verticalAlign='center'>
            <SwitchButton
              id='shieldsToggle'
              leftText='Up'
              rightText='Down'
              checked={false}
              onChange={shieldsToggled/* adBlockToggled */}
            />
          </Column>
          <Column size={2} align='flex-end' verticalAlign='center'>
            <ActionButton
              fontSize='20px'
              text='&times;'
              onClick={this.onClosePopup}
            />
          </Column>
          <Column>
            <Separator />
            <BrowserText noSelect text='Site shield settings for' />
          </Column>
          <Column verticalAlign='center'>
            <BrowserText noSelect fontSize='20px' text={hostname} />
          </Column>
        </Grid>
      </div>
    )
  }
}
