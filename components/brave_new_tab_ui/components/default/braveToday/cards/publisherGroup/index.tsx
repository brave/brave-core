// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Feature-specific components
import * as Card from './style'
import PublisherMeta from '../PublisherMeta'

interface Props {
  content: BraveToday.Article[]
  publisher: BraveToday.Publisher
  onReadFeedItem: (item: BraveToday.FeedItem) => any
}

export default function PublisherGroup(props: Props) {
  // No content no render®
  if (props.content.length < 3) {
    return null
  }
  return (
    <Card.OrderedList>
      <PublisherMeta
        publisher={props.publisher}
        title={true}
      />
      <Card.List>
        {
          props.content.map((item, index) => {
            if (item === undefined) {
              return null
            }
            return (
              <Card.ListItem key={index}>
                <a onClick={() => this.props.onReadFeedItem(item)} href={item.url}>
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
