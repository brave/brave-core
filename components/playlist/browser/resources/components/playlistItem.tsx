/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

interface Props {
    id: string
    name: string
    thumbnailUrl: string

    onClick: (id: string) => void
}

export default class PlaylistItem extends React.PureComponent<Props, {}> {
  render () {
    let { id, name, thumbnailUrl, onClick } = this.props
    return (
        <div>
            <h3>{name}</h3>
            <a href='#' onClick={() => { onClick(id) }}>
            <img
                style={{ maxWidth: '100px' }}
                data-id={id}
                src={thumbnailUrl}
            />
            </a>
        </div>
    )
  }
}
