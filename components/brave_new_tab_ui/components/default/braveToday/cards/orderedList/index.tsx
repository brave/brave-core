// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Feature-specific components
import * as Card from './style'
import PublisherMeta from '../PublisherMeta'

interface Props {
  content: (BraveToday.Article | undefined)[]
  publishers: BraveToday.Publishers
}

class CardOrderedList extends React.PureComponent<Props, {}> {
  render () {
    const { content }: Props = this.props

    // No content no render®
    if (content.length < 3) {
      return null
    }

    // @ts-ignore
    const publisherId = content && content[0] && content[0].publisher_id
    let allSamePublisher = false
    if (publisherId) {
      if (
        // @ts-ignore
        (content && content[1] && content[1].publisher_id) &&
        // @ts-ignore
        (content && content[1] && content[1].publisher_id) ===
        // @ts-ignore
        (content && content[2] && content[2].publisher_id)
      ) {
        allSamePublisher = true
      }
    }

    return (
      <Card.OrderedList>
        {
          // If all posts are from the same publisher,
          // show their logo
          allSamePublisher && publisherId &&
            <PublisherMeta
              publisher={this.props.publishers[publisherId]}
              title={true}
            />
        }
        <Card.List>
        {
          content.map((item, index) => {
            if (item === undefined) {
              return null
            }
            return (
              <Card.ListItem key={`card-ordered-list-key-${index}`}>
                <a href={item.url}>
                  <Card.Heading>
                    {item.title}
                  </Card.Heading>
                  <Card.Time>{item.relative_time}</Card.Time>
                </a>
              </Card.ListItem>
            )
          })
        }
        </Card.List>
      </Card.OrderedList>
    )
  }
}

export default CardOrderedList
