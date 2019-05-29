/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { connect } from 'react-redux'

// Components
import {
  WalletContainer,
  WalletInner,
  WalletTitle
} from './style'
import { WalletInfoIcon } from 'brave-ui/components/icons'

interface Props {
  walletData: Wallet.State
}

export interface State {
}

export class WalletPage extends React.Component<Props, State> {
  componentDidMount () {
    console.log('Wallet mounted')
  }

  render () {
    return(
      <WalletContainer>
        <WalletInner>
          <WalletInfoIcon />
          <WalletTitle>{'Brave Wallet'}</WalletTitle>
        </WalletInner>
      </WalletContainer>
    )
  }
}

export const mapStateToProps = (state: Wallet.ApplicationState) => ({
  walletData: state.walletData
})

export default connect(
  mapStateToProps
)(WalletPage)
