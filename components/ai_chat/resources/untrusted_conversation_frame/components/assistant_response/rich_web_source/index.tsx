// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as Mojom from '../../../../common/mojom'
import { Weather } from './weather'
import { WeatherResult } from './weather/types'

export default function RichWebSource(props: { data: Mojom.RichWebSource }) {
  if (props.data.type !== Mojom.RichWebSourceType.WEATHER || !props.data.data) {
    console.warn('RichWebSource: unsupported type', props.data)
    return null
  }

  // TODO: error boundary
  return <Weather data={JSON.parse(props.data.data) as unknown as WeatherResult} />
}
