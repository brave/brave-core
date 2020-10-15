// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react';
import CardImage from '../CardImage'
import * as Card from '../../cardSizes'
import PublisherMeta from '../PublisherMeta'

interface Props {
  content: (BraveToday.Article)[]
  publishers: BraveToday.Publishers
}

export default function CardSingleArticleMedium (props: Props) {
  const { content }: Props = props

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
          const publisher = props.publishers[item.publisher_id]
          return (
            <Card.Small key={`card-smallest-key-${index}`}>
              <a href={item.url}>
                <CardImage size="medium" imageUrl={item.img} />
                <Card.Content>
                  <Card.Text>
                    {item.title}
                    <Card.Time>{item.relative_time}</Card.Time>
                    {
                      publisher &&
                        <PublisherMeta publisher={publisher} />
                    }
                  </Card.Text>
                </Card.Content>
              </a>
            </Card.Small>
          )
        })
      }
    </Card.ContainerForTwo>
  )
}
