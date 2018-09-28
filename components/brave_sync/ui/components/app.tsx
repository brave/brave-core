/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Utils
import { getLocale } from '../../../common/locale'
import * as syncActions from '../actions/sync_actions'

// Assets
require('../../../fonts/muli.css')
require('../../../fonts/poppins.css')
require('emptykit.css')

interface Props {
  syncData: Sync.State
  actions: any
}

interface State {
  deviceName: string
  showQRCode: boolean
  showSyncWords: boolean
}

const syncLink = 'https://github.com/brave/sync/wiki/Design'

class SyncPage extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      deviceName: '',
      showQRCode: false,
      showSyncWords: false
    }
  }

  onGetUserInputDeviceName = (e: React.ChangeEvent<HTMLInputElement>) => {
    this.setState({ deviceName: e.target.value })
  }

  onSetupNewToSync = () => {
    this.props.actions.onSetupNewToSync(this.state.deviceName)
  }

  onRequestQRCode = () => {
    this.props.actions.onRequestQRCode()
    this.setState({ showQRCode: true })
  }

  onRequestSyncWords = () => {
    this.props.actions.onRequestSyncWords()
    this.setState({ showSyncWords: true })
  }

  onSyncReset = () => {
    this.props.actions.onSyncReset()
  }

  onToggleSyncThisDevice = (event: React.ChangeEvent<HTMLInputElement>) => {
    this.props.actions.onToggleSyncThisDevice(event.target.checked)
  }

  onSyncBookmarks = (event: React.ChangeEvent<HTMLInputElement>) => {
    this.props.actions.onSyncBookmarks(event.target.checked)
  }

  render () {
    const { syncData } = this.props
    if (!syncData) {
      return null
    }
    return (
      <div id='syncPage'>
        <h1>{getLocale('sync')}</h1>
        <p>
          {getLocale('syncInfo1')}&snbp;
          <a href={syncLink} target='_blank' rel='noreferrer noopener'>?</a>
        </p>
        <p>{getLocale('syncInfo2')}</p>
        {
          syncData.isSyncConfigured
            ? (
              <div style={{background: 'darkgreen', color: 'white'}}>
              <h1>Hello, your device named {syncData.thisDeviceName} was configured!</h1>
              <button onClick={this.onRequestQRCode}>Request QR Code</button>
              <button onClick={this.onRequestSyncWords}>Request Sync Words</button>
              <button onClick={this.onSyncReset}>RESET SYNC!!!!</button>
              <input id='syncMe' onChange={this.onToggleSyncThisDevice} type='checkbox' checked={syncData.shouldSyncThisDevice}/><label htmlFor='syncMe'>Keep syncing this device?</label>
              <ul>
                <li><input id='bookmarks' onChange={this.onSyncBookmarks}  type='checkbox' checked={syncData.syncBookmarks} /><label htmlFor='bookmarks'>Sync bookmarks?</label></li>
              </ul>
              {
                this.state.showQRCode
                  ? <div><img src={syncData.seedQRImageSource} /></div>
                  : null
              }
              {
                this.state.showSyncWords
                  ? <div>{syncData.syncWords}</div>
                  : null
              }
              </div>
            )
            : (
              <div style={{background: 'darkblue', color: 'white'}}>
                <input type='text' onChange={this.onGetUserInputDeviceName} />
                <button onClick={this.onSetupNewToSync}>Setup Sync</button>
              </div>
            )
        }
        <footer style={{background: 'black', color: 'white'}}>
          {JSON.stringify(syncData)}
        </footer>
      </div>
    )
  }
}

export const mapStateToProps = (state: Sync.ApplicationState) => ({
  syncData: state.syncData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(syncActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(SyncPage)
