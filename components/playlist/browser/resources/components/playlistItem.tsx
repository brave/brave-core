/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import styled, { css } from 'styled-components'

import ThumbnailIcon from '../assets/playlist-thumbnail-icon.svg'

interface Props {
    id: string
    name: string
    thumbnailUrl: string

    onClick: (id: string) => void
}

const StyledThumbnail = styled.img<{showDefaultThumbnail: boolean}>`
  max-width: 100%;

  ${p => p.showDefaultThumbnail && css`
    &::before { content: url(${ThumbnailIcon}); }  
  `}
`

export default class PlaylistItem extends React.PureComponent<Props, {}> {
  render () {
    let { id, name, thumbnailUrl, onClick } = this.props
    return (
        <div className='playlist-item'>
            <h3 className='playlist-item-name'>{name}</h3>
            <a href='#' onClick={() => { onClick(id) }}>
              <StyledThumbnail className='playlist-item-thumbnail' showDefaultThumbnail={!thumbnailUrl} data-id={id} src={thumbnailUrl}/>
            </a>
        </div>
    )
  }
}
