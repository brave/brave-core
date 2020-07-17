// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Feature-specific components
// Utils
import { generateRelativeTimeFormat } from '../../../../helpers/braveTodayUtils'
import * as Card from '../../cardSizes'
import { Debugger } from '../../default'

interface Props {
  content: (BraveToday.Article | BraveToday.Deal | undefined)[]
}

class CardSingleArticleSmall extends React.PureComponent<Props, {}> {
  render () {
    const { content }: Props = this.props

    // no full content no render®
    if (content.length < 3) {
      return null
    }

    return (
      <Card.ContainerForThree>
        {
          content.map((item, index) => {
            // If there is a missing item, return nothing
            if (item === undefined) {
              return null
            }
            return (
              <Card.Small key={`card-smallest-key-${index}`}>
                <a href={item.url}>
                  {
                    item.content_type === 'product'
                      ? <Debugger>this comes from a sponsor</Debugger>
                      : null
                  }
                  <Card.Image size='small' src={item.img} alt='' />
                  <Card.Content>
                    <Card.Text>
                      {item.title}
                      <Card.Time>{generateRelativeTimeFormat(item.publish_time)}</Card.Time>
                    </Card.Text>
                  </Card.Content>
                </a>
              </Card.Small>
            )
          })
        }
      </Card.ContainerForThree>
    )
  }
}

export default CardSingleArticleSmall
