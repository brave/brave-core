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
    this.getPlaylist()

    chrome.bravePlaylists.onPlaylistsChanged.addListener((changeType, id) => {
      // cc mark. this shows the change type and the id of the changed video
      // as expected, but does not update the playlist `partialReady` attr
      // audio/video are not often updated, even with the file available on disk
      console.log('changeType:', changeType, 'id', id)
      chrome.bravePlaylists.getPlaylist(id, (something: any) => {
        console.time('changeType')
        console.log('something', something)
        console.timeEnd('changeType')
      })
      this.getPlaylist()
    })
  }

  componentDidUpdate (prevProps: any, prevState: any) {
    if (JSON.stringify(prevState.playlists) !== JSON.stringify(this.state.playlists)) {
      this.getPlaylist()
    }
  }

  getPlaylistHeader = (): Cell[] => {
    return [
      { content: 'INDEX' },
      { content: 'NAME' },
      { content: 'STATUS' },
      { content: 'REMOVE' }
    ]
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

  getPlaylistRows = (playlist?: any): Row[] | undefined => {
    if (playlist == null) {
      return
    }

    return playlist.map((item: any, index: any): any => {
      console.log('video file', item.videoMediaFilePath)
      console.log('audio file', item.audioMediaFilePath)
      const cell: Row = {
        content: [
          { content: (<div style={{ textAlign: 'center' }}>{index}</div>) },
          { content: (
            <div>
              {
                item.audioMediaFilePath && item.videoMediaFilePath
                  ? (
                    <>
                      <video
                        controls={true}
                        muted={true}
                        width={640}
                        height={320}
                        poster={item.thumbnailUrl}
                      >
                        <source src={item.videoMediaFilePath} type='video/mp4' />
                      </video>
                      <audio controls={true}>
                        <source src={item.audioMediaFilePath} type='audio/mp4' />
                      </audio>
                    </>
                  )
                  : <h2>Video is being downloaded. It will show here once available</h2>
              }
              <h3>{item.playlistName}</h3>
            </div>
          ) },
          { content: (<span>{item.audioMediaFilePath && item.videoMediaFilePath ? 'Completed' : 'In progress'}</span>) },
          { content: (<button style={this.lazyButtonStyle} onClick={this.onClickRemoveVideo}><CloseCircleOIcon /></button>) }
        ]
      }
      return cell
    })
  }

  onClickRemoveALlVideos = () => {
    console.log('nothing')
  }

  onClickRemoveVideo = () => {
    console.log('nothing')
  }

  onClickPlayVideo = () => {
    console.log('nothing')
  }

  render () {
    const { actions, adblockData } = this.props
    const { playlists } = this.state

    // chrome.bravePlaylists.onPlaylistsChanged.addListener((changeType, id) => {
    //   // cc mark. this shows the change type and the id of the changed video
    //   // as expected, but does not update the playlist `partialReady` atrr
    //   // and no audio/video/thumbnail is shown
    //   console.log('changeType:', changeType, 'id', id)
    //   chrome.bravePlaylists.getPlaylist(id, (something: any) => {
    //     console.log('something', something)
    //   })
    // })

    return (
      <div id='adblockPage'>
        <div style={{ minHeight: '600px', width: '1200px' }}>
          <button
            style={
              Object.assign(
                {},
                this.lazyButtonStyle,
                { width: 'fit-content', fontSize: '16px' }
              )
            }
          >
            Delete all playlists
          </button>
          <Table header={this.getPlaylistHeader()} rows={this.getPlaylistRows(playlists)}>
            YOUR PLAYLIST IS NOW EMPTY
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
