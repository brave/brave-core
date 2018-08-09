/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { Table } from 'brave-ui/components'
import { Cell, Row } from 'brave-ui/components/dataTables/table/index.tsx'
import { Heading } from 'brave-ui/old'

// Constants
import { File, TorrentObj } from '../constants/webtorrentState'

interface Props {
  torrentId: string
  torrent?: TorrentObj
}

export default class TorrentFileList extends React.PureComponent<Props, {}> {
  render () {
    const { torrent } = this.props
    if (!torrent || !torrent.files) {
      return (
        <div>
          Click "Start Torrent" to load the torrent file list
        </div>
      )
    }

    const header: Cell[] = [
      {
        content: '#'
      },
      {
        content: 'Name'
      },
      {
        content: 'Save File'
      },
      {
        content: 'Size'
      }
    ]

    const renderFileLink = (file: File, ix: number, isDownload: boolean) => {
      const { torrentId, torrent } = this.props
      if (!torrent) return null
      if (isDownload) {
        if (torrent.serverURL) {
          const url = torrent.serverURL + '/' + ix
          return (<a href={url} download={file.name}>⇩</a>)
        } else {
          return (<div />) // No download links until the server is ready
        }
      } else {
        const href = torrentId + '&ix=' + ix
        return (<a href={href} target='_blank'> {file.name} </a>)
      }
    }

    const rows: Row[] = torrent.files.map((file: File, index: number) => {
      return ({
        content: [
          {
            content: index + 1
          },
          {
            content: renderFileLink(file, index, false)
          },
          {
            content: renderFileLink(file, index, true)
          },
          {
            content: file.length
          }
        ]
      })
    })

    return (
      <div>
        <Heading
          text='Files'
          level={3}
        />
        <Table
          header={header}
          rows={rows}
        >
        'Loading the torrent file list...'
        </Table>
      </div>
    )
  }
}
