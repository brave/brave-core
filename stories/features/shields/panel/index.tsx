/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import BraveShieldsHeader from './components/header'
import BraveShieldsStats from './components/stats'
import BraveShieldsControls from './components/controls'
import BraveShieldsFooter from './components/footer'

export default class BraveShields extends React.PureComponent {
  render () {
    return (
      <div style={{maxWidth: '300px'}}>
        <BraveShieldsHeader />
        <BraveShieldsStats />
        <BraveShieldsControls />
        <BraveShieldsFooter />
      </div>
    )
  }
}
