// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Feature-specific components
import * as Card from '../../../../components/default/braveToday/cardSizes'
import { Debugger } from '../../../../components/default/braveToday/default'

// Utils
import { generateRelativeTimeFormat } from '../../../../helpers/braveTodayUtils'

interface Props {
  content: (BraveToday.Article | undefined)[]
}

class CardSingleArticleLarge extends React.PureComponent<Props, {}> {
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
          <a href={item.url}>
            {
              item.content_type === 'product'
                ? <Debugger>this comes from a sponsor</Debugger>
                : null
            }
            <Card.Image
              size='large'
              src={item.img || ''}
              alt=''
            />
            <Card.Content>
              <Card.Heading>
                {item.title}
              </Card.Heading>
              <Card.Time>{generateRelativeTimeFormat(item.publish_time)}</Card.Time>
              {
                item.publisher_logo !== ''
                ? (
                  <Card.PublisherLogo
                    src={item.publisher_logo}
                    alt={item.publisher_id}
                  />
                ) : null
              }
            </Card.Content>
          </a>
        </Card.Large>
      )
    })
  }
}

export default CardSingleArticleLarge
