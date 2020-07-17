// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Feature-specific components
import * as Card from '../../cardSizes'
import { Debugger } from '../../default'

interface Props {
  content: (BraveToday.Deal | undefined)[]
}

class CardDealLarge extends React.PureComponent<Props, {}> {
  render () {
    const { content }: Props = this.props

    // no full content no render®
    if (content.length === 0) {
      return null
    }

    return content.map((item, index) => {
      // If there is a missing item, return nothing
      if (item === undefined) {
        return null
      }
      return (
        <Card.Large key={`card-key-${index}`}>
          <Debugger>this is a DEAL type</Debugger>
          <Card.Content>
            <Card.Heading>
              {/* <a href={item.url}>{item.title}</a> */}
            </Card.Heading>
            {/* <Card.Time>{item.publish_time}</Card.Time> */}
          </Card.Content>
        </Card.Large>
      )
    })
  }
}

export default CardDealLarge
