/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { bindActionCreators, Dispatch } from 'redux'
import { connect } from 'react-redux'

// Components
import MediaViewer from './mediaViewer'
import TorrentViewer from './torrentViewer'

// Constants
import {
  TorrentObj,
  TorrentState,
  ApplicationState,
  getTorrentObj,
  getTorrentState
} from '../constants/webtorrentState'

// Utils
import * as torrentActions from '../actions/webtorrent_actions'

interface Props {
  torrentState: TorrentState
  torrentObj?: TorrentObj
  actions: any
}

export class BraveWebtorrentPage extends React.Component<Props, {}> {
  render () {
    const { actions, torrentState, torrentObj } = this.props
    let torrentId = decodeURIComponent(window.location.search.substring(1))
    torrentId = window.location.hash
      ? torrentId + window.location.hash
      : torrentId

    if (!torrentState) return null

    let name = torrentObj && torrentObj.name
      ? torrentObj.name
      : torrentState && torrentState.name
      ? torrentState.name
      : undefined

    document.title = name
      ? name + ' – WebTorrent'
      : 'WebTorrent'

    if (torrentObj && typeof torrentState.ix === 'number') {
      return <MediaViewer torrent={torrentObj} ix={torrentState.ix} />
    }

    return (
      <TorrentViewer
        actions={actions}
        name={name}
        torrentId={torrentState.torrentId}
        errorMsg={torrentState.errorMsg}
        torrent={torrentObj}
        tabId={torrentState.tabId}
      />
    )
  }
}

export const mapStateToProps = (
  state: ApplicationState,
  ownProps: { tabId: number }
) => {
  return {
    torrentState: getTorrentState(state.torrentsData, ownProps.tabId),
    torrentObj: getTorrentObj(state.torrentsData, ownProps.tabId)
  }
}

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(torrentActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(BraveWebtorrentPage)
