// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Feature-specific components
import * as Card from '../../cardMedia'
import { Debugger } from '../../default'

interface Props {
  content: BraveToday.Article | undefined
}

class CardMedia extends React.PureComponent<Props, {}> {
  render () {
    const { content } = this.props

    if (!content) {
      return (<h1 style={{ color: 'red' }}>MEDIA MISSING</h1>)
    }

    return (
        <Card.MediaLink>
          <Debugger>this is a MEDIA type</Debugger>
          <a href={content.url} rel='nofererrer noopener'>
            <img alt='' />
          </a>
        </Card.MediaLink>
    )
  }
}

export default CardMedia
