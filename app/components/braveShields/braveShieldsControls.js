/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import React, { Component } from 'react'
import { Grid, Column, SwitchButton, BrowserSelect, ContentToggle } from 'brave-ui'

export default class BraveShieldsControls extends Component {
  constructor () {
    super()
    this.onChangeAdControl = this.onChangeAdControl.bind(this)
    this.onChangeCookieControl = this.onChangeCookieControl.bind(this)
    this.onToggleOpenShields = this.onToggleOpenShields.bind(this)
  }

  onChangeAdControl () {
    // TODO: @cezaraugusto

  }
  onChangeCookieControl () {
    // TODO: @cezaraugusto
  }

  onToggleOpenShields () {
    // The popup size is hardcoded in braveShieldsPanel.pug
    // to avoid flickering while opening.
    // since there's no way to reduce its size we then
    // set the body size per contentToggle setting

    // TODO: @cezaraugusto pass down toggleOpenShields
    // this just for checkbox testing
    this.props.adBlockToggled()
    this.props.adblock === 'allow'
      ? document.body.style.height = '587px'
      : document.body.style.height = '334px'
  }

  render () {
    // /* , trackingProtection, trackingProtectionToggled */
    const { adBlock, adBlockToggled } = this.props
    return (
      <Grid background='#eee' padding='10px 10px 0' gap='10px 5px'>
        <Column>
          <ContentToggle
            defaultOpen
            withSeparator
            summary='Advanced Controls'
            open={adBlock === 'allow'}
            onClick={this.onToggleOpenShields}
          >
            {/* TODO @cezaraugusto */}
            <BrowserSelect
              titleName='add control'
              value='someVALUE'
              onChange={this.onChangeAdControl}
            >
              <option value='SOME'>Show Brave Ads</option>
              <option value='SOME'>Block Ads</option>
              <option value='SOME'>Allow Ads and Tracking</option>
            </BrowserSelect>
            {/* TODO @cezaraugusto */}
            <BrowserSelect
              titleName='cookie control'
              value='someVALUE'
              onChange={this.onChangeCookieControl}
            >
              <option value='SOME'>Block 3rd Party Cookies</option>
              <option value='SOME'>Allow All Cookies</option>
              <option value='SOME'>Block All Cookies</option>
            </BrowserSelect>
            <Grid gap='10px 5px' padding='5px 0'>
              <Column>
                {/* TODO @cezaraugusto */}
                <SwitchButton
                  id='httpsEverywhere'
                  rightText='HTTPS Everywhere'
                  checked={adBlock === 'allow'}
                  onChange={adBlockToggled}
                />
              </Column>
              <Column>
                {/* TODO @cezaraugusto */}
                <SwitchButton
                  id='blockScripts'
                  rightText='Block Scripts'
                />
              </Column>
              <Column>
                {/* TODO @cezaraugusto */}
                <SwitchButton
                  id='fingerprintingProtection'
                  rightText='Fingerprinting Protection'
                />
              </Column>
              <Column>
                {/* TODO @cezaraugusto */}
                <SwitchButton
                  id='blockPhishingMalware'
                  rightText='Block Phishing/Malware'
                />
              </Column>
            </Grid>
          </ContentToggle>
        </Column>
      </Grid>
    )
  }
}
