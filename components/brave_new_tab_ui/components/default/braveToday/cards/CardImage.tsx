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

function useGetUnpaddedImage (paddedUrl: string, onLoaded?: () => any) {
  const [unpaddedUrl, setUnpaddedUrl] = React.useState('');
  const onReceiveUnpaddedUrl = (result: string) => {
    setUnpaddedUrl(result);
    window.requestAnimationFrame(() => {
      if (onLoaded)
        onLoaded()
    })
  }
  React.useEffect(() => {
    // Storybook method
    // @ts-ignore
    if (window.braveStorybookUnpadUrl) {
      // @ts-ignore
      window.braveStorybookUnpadUrl(paddedUrl)
      .then(onReceiveUnpaddedUrl)
      return
    }
    Background.send<
    BraveToday.Messages.GetImageDataResponse,
      BraveToday.Messages.GetImageDataPayload
    >(
      Background.MessageTypes.Today.getImageData,
      { url: paddedUrl }
    ).then(result => onReceiveUnpaddedUrl(result.dataUrl))
  }, [paddedUrl]);
  return unpaddedUrl
}

  return <Card.Image fit={props.fit} size={props.size} src={data} />
}