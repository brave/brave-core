/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { ShieldsPanel } from '../../../src/features/shields'
import BraveShieldsHeader from './components/header'
import BraveShieldsInterfaceControls from './components/interfaceControls'
import BraveShieldsPrivacyControls from './components/privacyControls'
import BraveShieldsSecurityControls from './components/securityControls'
import BraveShieldsFooter from './components/footer'

interface BraveShieldsProps {
  enabled: boolean
  fakeOnChange: () => void
}

export default class Shields extends React.PureComponent<BraveShieldsProps, {}> {
  render () {
    const { fakeOnChange, enabled } = this.props
    return (
      <ShieldsPanel enabled={enabled} style={{ width: '330px' }}>
        <BraveShieldsHeader fakeOnChange={fakeOnChange} enabled={enabled} />
        <BraveShieldsInterfaceControls enabled={enabled} />
        <BraveShieldsPrivacyControls enabled={enabled} />
        <BraveShieldsSecurityControls enabled={enabled} />
        <BraveShieldsFooter />
      </ShieldsPanel>
    )
  }
}
