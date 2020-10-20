// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Card from '../cardSizes'
import * as styled from '../default'

type Props = {
  publisher: BraveToday.Publisher
  title?: boolean
}

export default function PublisherMeta (props: Props) {
  const Component = props.title
    ? Card.Heading
    : styled.PublisherName
  return (
    <Component>
      {props.publisher.publisher_name}
    </Component>
  )
}
