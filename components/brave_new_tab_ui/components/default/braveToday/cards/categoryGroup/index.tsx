// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import CardImage from '../CardImage'
import * as Card from './style'

interface Props {
  content: (BraveToday.Article)[]
  publishers: BraveToday.Publishers
  categoryName: string
  onReadFeedItem: (item: BraveToday.FeedItem) => any
}

export default function CategoryGroup (props: Props) {
  // No content no render®
  if (props.content.length < 3) {
    return null
  }
  return (
    <Card.BrandedList>
      <Card.Title>{ props.categoryName }</Card.Title>
      <Card.List>
        {
          props.content.map((item, index) => {
            const publisher = props.publishers[item.publisher_id]
            return (
              <Card.ListItem key={index}>
                <a onClick={() => this.props.onReadFeedItem(item)} href={item.url}>
                  <Card.Content>
                    <Card.Heading>{publisher.publisher_name}</Card.Heading>
                    <Card.Heading>{item.title}</Card.Heading>
                    <Card.Time>{item.relative_time}</Card.Time>
                  </Card.Content>
                  <Card.ListItemImageFrame>
                    <CardImage list={true} imageUrl={item.img} />
                  </Card.ListItemImageFrame>
                </a>
              </Card.ListItem>
            )
          })
        }
      </Card.List>
    </Card.BrandedList>
    )
}
