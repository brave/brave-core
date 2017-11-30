/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import React, { Component } from 'react'
import { Grid, Column, Separator, SwitchButton, ActionButton, BrowserText } from 'brave-ui'

export default class BraveShieldsHeader extends Component {
  constructor () {
    super()
    this.onToggleShields = this.onToggleShields.bind(this)
    this.onClosePopup = this.onClosePopup.bind(this)
  }

  onToggleShields (e) {
    const shieldsOption = e.target.checked ? 'allow' : 'block'
    this.props.shieldsToggled(shieldsOption)
  }

  onClosePopup () {
    window.close()
  }

  render () {
    const { shieldsEnabled, hostname } = this.props
    return (
      <Grid id='braveShieldsHeader' background='#808080' padding='10px' gap='0' textColor='#fafafa'>
        <Column size={4} verticalAlign='center'>
          <BrowserText noSelect fontSize='14px' text='Shields' />
        </Column>
        <Column size={6} verticalAlign='center'>
          <SwitchButton
            id='shieldsToggle'
            leftText='Down'
            rightText='Up'
            checked={shieldsEnabled === 'allow'}
            onChange={this.onToggleShields}
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
    )
  }
}
