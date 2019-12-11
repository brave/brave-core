/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import { AdBlockItemList } from './adBlockItemList'
import { CustomFilters } from './customFilters'
import { NumBlockedStat } from './numBlockedStat'

// Components for playlist
import { CloseCircleOIcon } from 'brave-ui/components/icons'
import Table, { Cell, Row } from 'brave-ui/components/dataTables/table'

// Utils
import * as adblockActions from '../actions/adblock_actions'

interface Props {
  actions: any
  adblockData: AdBlock.State
}

interface State {
  playlists: any
}

export class AdblockPage extends React.Component<Props, State> {
  constructor (props: Props) {
    super(props)
    this.state = { playlists: [] }
    this.getPlaylist()
  }

  get actions () {
    return this.props.actions
  }

  getPlaylist = () => {
    chrome.bravePlaylists.getAllPlaylists(playlists => {
      this.setState({ playlists })
    })
  }

  componentDidMount () {
    chrome.bravePlaylists.onPlaylistsChanged.addListener((changeType, id) => {
      this.getPlaylist()
    })
  }

  componentDidUpdate (prevProps: any, prevState: any) {
    if (JSON.stringify(prevState.playlists) !== JSON.stringify(this.state.playlists)) {
      this.getPlaylist()
    }
  }

  getImgSrc = (playlistId: string) => {
    return 'chrome://playlists-image/' + playlistId
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
          { content: (<div style={{ textAlign: 'center' }}>{index+1}</div>) },
          { content: (
            <div>
              <h3>{item.playlistName}</h3>
              <a href='#' onClick={this.onClickPlayVideo.bind(this, item.id)}>
                <img style={{ maxWidth: '200px' }}
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

  onClickPlayVideo = (playlistId: string) => {
    chrome.bravePlaylists.play(playlistId)
  }

  onClickRemoveVideo = (playlistId: string) => {
    chrome.bravePlaylists.deletePlaylist(playlistId)
  }

  render () {
    const { actions, adblockData } = this.props
    const { playlists } = this.state
    return (
      <div id='adblockPage'>
        <div style={{ minHeight: '600px', width: '1200px' }}>
          <Table header={this.getPlaylistHeader()} rows={this.getPlaylistRows(playlists)}>
            YOUR PLAYLIST IS EMPTY
          </Table>
        </div>
        <hr />
        <NumBlockedStat adsBlockedStat={adblockData.stats.adsBlockedStat || 0} />
        <AdBlockItemList
          actions={actions}
          resources={adblockData.settings.regionalLists}
        />
        <CustomFilters
          actions={actions}
          rules={adblockData.settings.customFilters || ''}
        />
      </div>
    )
  }
}

export const mapStateToProps = (state: AdBlock.ApplicationState) => ({
  adblockData: state.adblockData
})

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(adblockActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(AdblockPage)
