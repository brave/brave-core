/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { ConnectedPeers } from './connectedPeers'

// Utils
import * as ipfsActions from '../actions/ipfs_actions'

interface Props {
  actions: any
  ipfsData: IPFS.State
}

export class IPFSPage extends React.Component<Props, {}> {
  constructor (props: Props) {
    super(props)
  }

  get actions () {
    return this.props.actions
  }

  render () {
    return (
      <div id='ipfsPage'>
        <ConnectedPeers peerCount={this.props.ipfsData.connectedPeers.peerCount} />
      </div>
    )
  }
}

export const mapStateToProps = (state: IPFS.ApplicationState) => ({
  ipfsData: state.ipfsData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(ipfsActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(IPFSPage)
