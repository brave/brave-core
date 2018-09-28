/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Feature-specific components
import {
  Main,
  Title,
  EmphasisText,
  SecondaryText,
  Link,
  SectionBlock
} from 'brave-ui/features/sync'

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
  syncWords: string
}

const syncLink = 'https://github.com/brave/sync/wiki/Design'

export class SyncPage extends React.PureComponent<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = {
      deviceName: '',
      showQRCode: false,
      showSyncWords: false,
      syncWords: ''
    }
  }

  componentDidMount = () => {
    // Inform the back-end that Sync can be loaded
    syncActions.onPageLoaded()
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

  onGetUserInputSyncWords = (event: React.ChangeEvent<HTMLTextAreaElement>) => {
    this.setState({ syncWords: event.target.value })
  }

  onRequestSyncWords = () => {
    this.props.actions.onRequestSyncWords()
    this.setState({ showSyncWords: true })
  }

  onSetupSyncHaveCode = () => {
    const { deviceName, syncWords } = this.state
    this.props.actions.onSetupSyncHaveCode(syncWords, deviceName)
  }

  onRemoveDevice = (event: React.MouseEvent<HTMLButtonElement>) => {
    const target = event.target as HTMLButtonElement
    this.props.actions.onRemoveDevice(target.id)
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

  onSyncSavedSiteSettings = (event: React.ChangeEvent<HTMLInputElement>) => {
    this.props.actions.onSyncSavedSiteSettings(event.target.checked)
  }

  onSyncBrowsingHistory = (event: React.ChangeEvent<HTMLInputElement>) => {
    this.props.actions.onSyncBrowsingHistory(event.target.checked)
  }

  render () {
    const { syncData } = this.props
    if (!syncData) {
      return null
    }
    return (
      <div id='syncPage'>
        <Main>
          <Title level={2}>{getLocale('sync')}</Title>
            <EmphasisText>
              {getLocale('syncInfo1')}
              <Link href={syncLink} target='_blank' rel='noreferrer noopener'>?</Link>
            </EmphasisText>
            <SecondaryText>{getLocale('syncInfo2')}</SecondaryText>
          <SectionBlock>
            {
              syncData.isSyncConfigured
                ? (
                  <div style={{background: 'darkgreen', color: 'white'}}>
                  <h1>Hello, your device named {syncData.thisDeviceName} was configured!</h1>
                  <button onClick={this.onRequestQRCode}>Request QR Code</button>
                  <button onClick={this.onRequestSyncWords}>Request Sync Words</button>
                  <button onClick={this.onSyncReset}>RESET SYNC!!!!</button>
                  <input id='syncMe' onChange={this.onToggleSyncThisDevice} type='checkbox' checked={syncData.shouldSyncThisDevice} /><label htmlFor='syncMe'>Keep syncing this device?</label>
                  <ul>
                    <li><input id='bookmarks' onChange={this.onSyncBookmarks}  type='checkbox' checked={syncData.syncBookmarks} /><label htmlFor='bookmarks'>Sync bookmarks?</label></li>
                    <li><input id='siteSettings' onChange={this.onSyncSavedSiteSettings}  type='checkbox' checked={syncData.syncSavedSiteSettings} /><label htmlFor='siteSettings'>Sync site settings?</label></li>
                    <li><input id='browsingHistory' onChange={this.onSyncBrowsingHistory}  type='checkbox' checked={syncData.syncBrowsingHistory} /><label htmlFor='browsingHistory'>Sync browsing history?</label></li>
                  </ul>
                  <div style={{
                    color: 'gray',
                    display: 'grid',
                    height: '100%',
                    gridTemplateColumns: '1fr 1fr 1fr 1fr',
                    gridTemplateRows: '1fr',
                    gridGap: '15px'
                  }}>
                    {
                      syncData.devices.map((device: Sync.Devices) => {
                        return (
                          <>
                            <div key={`id-${device.id}`}>{device.id}</div>
                            <div key={`name-${device.id}`}>{device.name}</div>
                            <div key={`lastActive-${device.id}`}>{device.lastActive}</div>
                            <button id={device.id.toString()} key={`remove-${device.id}`} onClick={this.onRemoveDevice}>remove device</button>
                          </>
                        )
                      })
                    }
                  </div>
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
                  <>
                    <div style={{background: 'darkblue', color: 'white'}}>
                      <input type='text' onChange={this.onGetUserInputDeviceName} />
                      <button onClick={this.onSetupNewToSync}>Setup Sync</button>
                    </div>
                    <div style={{background: 'lightyellow'}}>
                      <textarea onChange={this.onGetUserInputSyncWords} />
                      <button onClick={this.onSetupSyncHaveCode}>
                        I have an existing sync code!
                      </button>
                    </div>
                  </>
                )
            }
          </SectionBlock>
        </Main>
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
