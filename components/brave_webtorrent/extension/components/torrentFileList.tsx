/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { Button, Table } from 'brave-ui/components'
import { Cell, Row } from 'brave-ui/components/dataTables/table/index'
import * as prettierBytes from 'prettier-bytes'
import * as React from 'react'
import styled from 'styled-components'
import { getMessage } from '../background/api/locale_api'
import { File, TorrentObj } from '../constants/webtorrentState'
import { getFileType } from '../utils/fileType'
import { Header } from './header'
import { Link } from './link'
import Spinner from './spinner'

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
  background: var(--background1);
  border-radius: 8px;
  padding: 16px;
  box-shadow: 0px 0.5px 1.5px 0px rgb(0 0 0 / 15%);
  
  td {
    border: none;
    color: var(--text1);
  }

  th {
    border-top: none;
    border-color: var(--divider1);
    border-width: 2px;
    color: var(--text3);
  }

  table {
    margin: 0;
    padding: 0;
  }
`

const LoadingContainer = styled.div`
  display: flex;
  flex-direction: column;
`

const SaveButton = styled(Button)`
  --button-main-color: var(--text1);
  --button-main-color-hover: var(--text2);
`

const tableHeader: Cell[] = [
  {
    content: '#'
  },
  {
    content: getMessage('filenameColumnHeader')
  },
  {
    content: getMessage('saveFileColumnHeader')
  },
  {
    content: getMessage('sizeColumnHeader')
  }
]

const downloadFile = (url: string, filename: string) => {
  const anchor = document.createElement('a')
  anchor.setAttribute('href', url)
  anchor.setAttribute('download', filename)
  anchor.click()
}

export default function TorrentFileList ({ torrent, torrentId, onSaveAllFiles }: Props) {
    const rows = React.useMemo<Row[] | undefined>(() => torrent?.files?.map((file: File, index: number) => ({
        content: [
          {
            content: index + 1
          },
          {
            content: getFileType(file) !== 'unknown'
              ? <Link target='_blank' rel='noopener'
                href={torrentId + (/^https?:/.test(torrentId)
                  ? '#ix='
                  : '&ix=') + index}>
                {file.name}
              </Link>
              : <span>{file.name}</span>
          },
          {
            content: torrent.serverURL && <SaveButton text={getMessage('saveFile')}
              level="secondary"
              onClick={() => downloadFile(`${torrent.serverURL}/${index}/${file.name}`, file.name)}/>
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
        <Header>{getMessage('filesHeader')}</Header>
        <Button onClick={saveAllFiles} text={getMessage('saveAllFiles')}/>
      </HeaderRow>
      <FilesContainer>
        <Table header={tableHeader} rows={rows}>
            {torrent && !torrent.files
              ? <LoadingContainer>
                <Spinner/>
                {getMessage('fileListLoading')}
              </LoadingContainer>
              : getMessage('startTorrentPrompt')}
          </Table>
      </FilesContainer>
    </Container>
}
