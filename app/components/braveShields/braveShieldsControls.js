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
    this.onToggleControls = this.onToggleControls.bind(this)
  }

  onChangeAdControl (e) {
    const { props } = this
    props.blockAdsTrackers(e.target.value)
  }

  onChangeCookieControl () {
    // TODO: @cezaraugusto
  }

  onToggleControls (e) {
    const { props } = this
    props.controlsToggled(!props.controlsOpen)
  }

  render () {
    const { shieldsEnabled, adsTrackers, controlsOpen } = this.props
    return (
      <Grid background='#eee' padding='10px 10px 0' gap='10px 5px'>
        <Column>
          <ContentToggle
            withSeparator
            open={controlsOpen}
            summary='Advanced Controls'
            onClick={this.onToggleControls}
          >
            <BrowserSelect
              disabled={shieldsEnabled === 'block'}
              titleName='Ad Control'
              value={adsTrackers}
              onChange={this.onChangeAdControl}
            >
              {/* TODO needs "show brave ads" */}
              <option value='allow'>Block Ads</option>
              <option value='block'>Allow Ads and Tracking</option>
            </BrowserSelect>
            {/* TODO @cezaraugusto */}
            <BrowserSelect
              disabled={shieldsEnabled === 'block'}
              titleName='Cookie Control'
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
                  disabled={shieldsEnabled === 'block'}
                  rightText='HTTPS Everywhere'
                />
              </Column>
              <Column>
                {/* TODO @cezaraugusto */}
                <SwitchButton
                  id='blockScripts'
                  disabled={shieldsEnabled === 'block'}
                  rightText='Block Scripts'
                />
              </Column>
              <Column>
                {/* TODO @cezaraugusto */}
                <SwitchButton
                  id='fingerprintingProtection'
                  disabled={shieldsEnabled === 'block'}
                  rightText='Fingerprinting Protection'
                />
              </Column>
              <Column>
                {/* TODO @cezaraugusto */}
                <SwitchButton
                  id='blockPhishingMalware'
                  disabled={shieldsEnabled === 'block'}
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
