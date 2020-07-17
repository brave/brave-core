// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Feature-specific components
import CardSmall from './_deals/cardDealMedium'
import CardSmallest from './_deals/cardDealSmall'
import CardMedium from './_deals/cardDealLarge'

interface Props {
  content: (BraveToday.Deal | undefined)[]
}

class CardDeals extends React.PureComponent<Props, {}> {
  render () {
    const { content }: Props = this.props

    // no full content no render®
    if (content.length === 0) {
      return null
    }

    if (content.length % 2 === 0) {
      return <CardSmall content={content} />
    }
    if (content.length % 3 === 0) {
      return <CardSmallest content={content} />
    }
    // Defaults to 1 large card at a time
    return <CardMedium content={content} />
  }
}

export default CardDeals
