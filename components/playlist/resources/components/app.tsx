/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components for playlist
import { CloseCircleOIcon } from 'brave-ui/components/icons'
import Table, { Cell, Row } from 'brave-ui/components/dataTables/table'

// Utils
import * as playlistActions from '../actions/playlist_actions'

interface Props {
  actions: any
  playlistData: Playlist.State
}

interface State {
  playlist: any,
  experimentalUrl: string
}

export class PlaylistPage extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = { playlist: [], experimentalUrl: '' }
    this.onClickDownloadVideo = this.onClickDownloadVideo.bind(this)
    this.getPlaylist()
  }

  get actions () {
    return this.props.actions
  }

  getPlaylist = () => {
    chrome.bravePlaylist.getAllPlaylistItems(playlistItems => {
      this.setState({ playlist: playlistItems })
    })
  }

  componentDidMount () {
    chrome.bravePlaylist.onPlaylistItemStatusChanged.addListener((changeType, id) => {
      this.getPlaylist()
    })
  }

  componentDidUpdate (prevProps: any, prevState: any) {
    if (JSON.stringify(prevState.playlist) !== JSON.stringify(this.state.playlist)) {
      this.getPlaylist()
    }
  }

  getImgSrc = (playlistId: string) => {
    return 'chrome://playlist-image/' + playlistId
  }

  get lazyButtonStyle () {
    const lazyButtonStyle: any = {
      alignItems: 'center',
      WebkitAppearance: 'none',
      width: '50px',
      height: '50px',
      display: 'flex',
      borderRadius: '4px'
    }
    return lazyButtonStyle
  }

  getPlaylistHeader = (): Cell[] => {
    return [
      { content: 'INDEX' },
      { content: 'NAME' },
      { content: 'STATUS' },
      { content: 'REMOVE' }
    ]
  }

  getPlaylistRows = (playlist?: any): Row[] | undefined => {
    if (playlist == null) {
      return
    }

    return playlist.map((item: any, index: any): any => {
      const cell: Row = {
        content: [
          { content: (<div style={{ textAlign: 'center' }}>{index + 1}</div>) },
          { content: (
            <div>
              <h3>{item.playlistName}</h3>
              <a href='#' onClick={this.onClickPlayVideo.bind(this, item.id)}>
                <img
                  style={{ maxWidth: '200px' }}
                  src={this.getImgSrc(item.id)}
                />
                </a>
            </div>
          ) },
          { content: (<span>{item.videoMediaFilePath ? 'Ready' : 'Downloading'}</span>) },
          { content: (<button style={this.lazyButtonStyle} onClick={this.onClickRemoveVideo.bind(this, item.id)}><CloseCircleOIcon /></button>) }
        ]
      }
      return cell
    })
  }

  onClickPlayVideo = (playlistItemId: string) => {
    chrome.bravePlaylist.playItem(playlistItemId)
  }

  onClickRemoveVideo = (playlistItemId: string) => {
    chrome.bravePlaylist.deletePlaylistItem(playlistItemId)
  }

  get pageHasDownloadableVideo () {
    return this.state.experimentalUrl.startsWith('https://www.youtube.com/watch')
  }

  onChangeExperimentalUrl = (event: React.ChangeEvent<HTMLTextAreaElement>) => {
    this.setState({ experimentalUrl: event.target.value })
  }

  onClickDownloadVideo = () => {
    chrome.bravePlaylist.requestDownload(this.state.experimentalUrl)
  }

  render () {
    const { playlist } = this.state
    return (
      <div id='playlistPage'>
        <div style={{ minHeight: '600px', width: '1200px' }}>
          <Table header={this.getPlaylistHeader()} rows={this.getPlaylistRows(playlist)}>
            YOUR PLAYLIST IS EMPTY
          </Table>
        </div>

        <div>
          <h1>Experimental</h1>
          <div>
            <textarea
              cols={100}
              rows={10}
              value={this.state.experimentalUrl}
              onChange={this.onChangeExperimentalUrl}
            />
            <div>
            {
              this.pageHasDownloadableVideo
              ? (
                <div>
                  <h1>This page has a video you can download</h1>
                  <button onClick={this.onClickDownloadVideo}>Click here to download</button>
                </div>
              ) : (
                <h1>Nothing to see here. Put a proper YouTube link to see the magic</h1>
              )
            }
            </div>
          </div>
        </div>
      </div>
    )
  }
}

export const mapStateToProps = (state: Playlist.ApplicationState) => ({
  playlistData: state.playlistData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(playlistActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(PlaylistPage)
