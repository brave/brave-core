// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import OptIn from './opt-in'

export default class LegacyApp extends React.PureComponent<{}, {}> {

  onWalletOptin = () => {
    chrome.braveWallet.loadUI(() => {
      window.location.href = 'chrome://wallet'
    })
  }

  render () {
    return (
      <OptIn onWalletOptIn={this.onWalletOptin} />
    )
  }
}
