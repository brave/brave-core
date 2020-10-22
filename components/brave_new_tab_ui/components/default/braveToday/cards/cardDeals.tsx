// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Feature-specific components
import * as Card from '../cardSizes'
import CardImage from './CardImage'

interface Props {
  content: BraveToday.Deal[]
}

export default function CardDeals (props: Props) {
  // no full content no render®
  if (props.content.length === 0) {
    return null
  }

  return (
    <Card.DealsCard>
      <Card.Heading>{props.content[0].offers_category}</Card.Heading>
      <Card.DealsList>
        {
          props.content.map((item, index) => {
            return (
              <Card.DealItem href={item.url} key={`card-smallest-key-${index}`}>
                <CardImage imageUrl={item.padded_img} />
                <Card.Text>{item.title}</Card.Text>
                <Card.Time>{item.description}</Card.Time>
              </Card.DealItem>
            )
          })
        }
      </Card.DealsList>
    </Card.DealsCard>
  )
}
