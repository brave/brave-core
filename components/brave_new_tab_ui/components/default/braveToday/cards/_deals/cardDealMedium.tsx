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

class CardDealMedium extends React.PureComponent<Props, {}> {
  render () {
    const { content }: Props = this.props

    // no full content no render®
    if (content.length < 2) {
      return null
    }

    return (
      <Card.ContainerForTwo>
        {
          content.map((item, index) => {
            // If there is a missing item, return nothing
            if (item === undefined) {
              return null
            }
            return (
              <Card.Small key={`card-smallest-key-${index}`}>
                <Debugger>this is a DEAL type</Debugger>
                <Card.Content>
                  <Card.Text>
                    {/* <a href={item.url}>{item.title}</a> */}
                    {/* <Card.Time>{item.publish_time}</Card.Time> */}
                  </Card.Text>
                </Card.Content>
              </Card.Small>
            )
          })
        }
      </Card.ContainerForTwo>
    )
  }
}

export default CardDealMedium
