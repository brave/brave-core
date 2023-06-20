// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useLazyUnpaddedImageUrl } from '../../../brave_new_tab_ui/components/default/braveNews/useUnpaddedImageUrl'
import styled from 'styled-components'

const Img = styled('img')`
  width: 100%;
  height: 100%;
  object-fit: cover;
`

export default function Image(props: { url: string }) {
  if (!props.url) return null

  const { setElementRef, url } = useLazyUnpaddedImageUrl(props.url, {})
  return <Img ref={setElementRef} src={url} />
}
