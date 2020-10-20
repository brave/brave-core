// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Card from '../cardSizes'
import * as Background from '../../../../../common/Background'

type Props = {
  size: string
  imageUrl: string
  onLoaded?: () => any
  fit?: boolean
}

export default function CardImage (props: Props) {
  const [data, setData] = React.useState('');

  React.useEffect(() => {
    Background.send<
    BraveToday.Messages.GetImageDataResponse,
      BraveToday.Messages.GetImageDataPayload
    >(
      Background.MessageTypes.Today.getImageData,
      { url: props.imageUrl }
    ).then(result => {
      setData(result.dataUrl);
      window.requestAnimationFrame(() => {
        if (props.onLoaded)
          props.onLoaded()
      })
    })
  });

  return <Card.Image fit={props.fit} size={props.size} src={data} />
}