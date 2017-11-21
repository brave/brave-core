/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import React, { Component } from 'react'

export default class BraveShieldsHeader extends Component {
  render () {
    const { hostname } = this.props
    return (
      <header>
        <h1>{hostname}</h1>
      </header>
    )
  }
}
