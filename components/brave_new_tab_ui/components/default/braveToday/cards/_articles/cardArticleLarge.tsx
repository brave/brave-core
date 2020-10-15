// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Feature-specific components
import * as Card from '../../cardSizes'
import CardImage from '../CardImage'
import PublisherMeta from '../PublisherMeta'

interface Props {
  content: (BraveToday.Article | undefined)[]
  publishers: BraveToday.Publishers
}

export default function CardSingleArticleLarge (props: Props) {
  // no full content no render®
  if (props.content.length === 0) {
    return <></>
  }

  return <>{props.content.map((item, index) => {
    // If there is a missing item, return nothing
    if (item === undefined) {
      return <></>
    }

    const publisher = props.publishers[item.publisher_id]

    return (
      <Card.Large key={`card-key-${index}`}>
        <a href={item.url}>
          <CardImage
            size='large'
            imageUrl={item.img}
          />
          <Card.Content>
            <Card.Heading>
              {item.title}
            </Card.Heading>
            <Card.Time>{item.relative_time}</Card.Time>
            {
              publisher &&
                <PublisherMeta publisher={publisher} />
            }
          </Card.Content>
        </a>
      </Card.Large>
    )
  })}</>
}
