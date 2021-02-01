/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { AddressesConfig } from './addressesConfig'
import { ConnectedPeers } from './connectedPeers'
import { DaemonStatus } from './daemonStatus'
import { NodeInfo } from './nodeInfo'
import { RepoStats } from './repoStats'
import { UninstalledView } from './uninstalledView'

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

  launchDaemon = () => {
    this.actions.launchDaemon()
  }

  shutdownDaemon = () => {
    this.actions.shutdownDaemon()
  }

  refreshActions = () => {
    this.actions.getConnectedPeers()
    this.actions.getAddressesConfig()
    this.actions.getRepoStats()
    this.actions.getNodeInfo()
  }

  componentDidUpdate (prevProps: Props) {
    if (prevProps.ipfsData.daemonStatus.launched !== this.props.ipfsData.daemonStatus.launched) {
      setTimeout(this.refreshActions, 2000)
    }
  }

  render () {
    if (!this.props.ipfsData.daemonStatus.installed) {
      return (
        <div id='ipfsPage'>
          <UninstalledView />
        </div>
      )
    }

    return (
      <div id='ipfsPage'>
        <DaemonStatus daemonStatus={this.props.ipfsData.daemonStatus} onLaunch={this.launchDaemon} onShutdown={this.shutdownDaemon}/>
        <ConnectedPeers peerCount={this.props.ipfsData.connectedPeers.peerCount} />
        <AddressesConfig addressesConfig={this.props.ipfsData.addressesConfig} />
        <RepoStats repoStats={this.props.ipfsData.repoStats} />
        <NodeInfo nodeInfo={this.props.ipfsData.nodeInfo} />
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
