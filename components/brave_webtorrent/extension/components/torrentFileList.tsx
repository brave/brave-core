/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Button, Table } from 'brave-ui/components'
import { Cell, Row } from 'brave-ui/components/dataTables/table/index'
import { LoaderIcon } from 'brave-ui/components/icons'
import * as prettierBytes from 'prettier-bytes'
import * as React from 'react'

// Constants
import styled from 'styled-components'
import { File, TorrentObj } from '../constants/webtorrentState'
import { Header } from './Header'
import { Link } from './Link'

interface Props {
  torrentId: string
  torrent?: TorrentObj
  onSaveAllFiles: () => void
}

const Container = styled.div`
  display: flex;
  flex-direction: column;
  gap: 16px;
`

const HeaderRow = styled.div`
  display: flex;
  justify-content: space-between;
`

const FilesContainer = styled.div`
  background: var(--background01);
  border-radius: 8px;
  padding: 16px;
  box-shadow: 0px 0.5px 1.5px 0px rgb(0 0 0 / 15%);
  
  td {
    border: none;
  }

  th {
    border-top: none!important;
  }

  table {
    margin: 0;
    padding: 0;
  }
`

const LoadingContainer = styled.div`
  display: flex;
  flex-direction: column;
  svg {
    height: 32px;
  }
`

const tableHeader: Cell[] = [
  {
    content: '#'
  },
  {
    content: 'Name'
  },
  {
    content: 'Save file'
  },
  {
    content: 'Size'
  }
]

export default function TorrentFileList ({ torrent, torrentId, onSaveAllFiles }: Props) {
    const rows = React.useMemo<Row[] | undefined>(() => torrent?.files?.map((file: File, index: number) => ({
        content: [
          {
            content: index + 1
          },
          {
            content: <Link target='_blank' rel='noopener'
              href={torrentId + (/^https?:/.test(torrentId)
                ? '#ix='
                : '&ix=') + index}>
              {` ${file.name} `}
            </Link>
          },
          {
            content: torrent.serverURL &&
              <Link href={`${torrent.serverURL}/${index}/${file.name}`} download={file.name}>⇩</Link>
          },
          {
            content: prettierBytes(file.length)
          }
        ]
      })), [torrent?.files, torrentId])

    const saveAllFiles = () => {
      if (!torrent?.serverURL || !torrent?.files || torrent?.progress !== 1) {
        return
      }
      onSaveAllFiles()
    }

    return <Container>
      <HeaderRow>
        <Header>Files</Header>
        <Button onClick={saveAllFiles} text="Save all files"/>
      </HeaderRow>
      <FilesContainer>
        <Table header={tableHeader} rows={rows}>
            {torrent && torrent.files
              ? <LoadingContainer>
                <LoaderIcon />
                Loading the torrent file list
              </LoadingContainer>
              : 'Click "Start Torrent" to begin your download.'}
          </Table>
      </FilesContainer>
    </Container>
}
