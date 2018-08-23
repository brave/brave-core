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
import { TorrentObj } from '../constants/webtorrentState'

// Utils
import * as torrentActions from '../actions/webtorrent_actions'
import { TorrentState, ApplicationState, getTorrentObj, getTorrentState } from '../constants/webtorrentState'

// Assets
require('../../../styles/webtorrent.less')

interface Props {
  torrentState: TorrentState
  torrentObj?: TorrentObj
  actions: any
}

export class BraveWebtorrentPage extends React.Component<Props, {}> {
  render() {
    const { actions, torrentState, torrentObj } = this.props
    const torrentId = decodeURIComponent(window.location.search.substring(1))

    // The active tab change might not be propagated here yet, so we might get
    // the old active tabId here which might be a different torrent page or a
    // non-torrent page.
    if (!torrentState || torrentId != torrentState.torrentId) {
      return (<div>Loading...</div>)
    }

    if (torrentObj && typeof(torrentState.ix) === 'number') {
      return (
        <MediaViewer
          torrent={torrentObj}
          ix={torrentState.ix}
        />)
    }

    return (
      <TorrentViewer
        actions={actions}
        name={torrentState.name}
        torrentId={torrentState.torrentId}
        errorMsg={torrentState.errorMsg}
        torrent={torrentObj}
        tabId={torrentState.tabId}
      />)
  }
}

export const mapStateToProps = (state: ApplicationState) => {
  return { torrentState: getTorrentState(state.torrentsData),
           torrentObj: getTorrentObj(state.torrentsData) }
}

export const mapDispatchToProps = (dispatch: Dispatch) => ({
  actions: bindActionCreators(torrentActions, dispatch)
})

export default connect(
  mapStateToProps,
  mapDispatchToProps
)(BraveWebtorrentPage)
