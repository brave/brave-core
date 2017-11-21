/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import React, { Component } from 'react'

export default class BraveShieldsControls extends Component {
  render () {
    const { adBlock, trackingProtection, toggleAdBlock, toggleTrackingProtection } = this.props
    return <header>
      <h1 onClick={toggleAdBlock}>adBlock: {adBlock}</h1>
      <h1 onClick={toggleTrackingProtection}>trackingProtection: {trackingProtection}</h1>
    </header>
  }
}
