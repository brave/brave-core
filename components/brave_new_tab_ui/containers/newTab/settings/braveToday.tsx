// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Components
import {
  SettingsRow,
  SettingsText
} from '../../../components/default'
import { Toggle } from '../../../components/toggle'

interface Props {
  publishers?: BraveToday.Publishers
}

export default function TodaySettings (props: Props) {
  if (!props.publishers) {
    return <h1>Loading...</h1>
  }
  return (
    <div>
      {Object.values(props.publishers).map(publisher => (
        <SettingsRow>
          <SettingsText>{publisher.publisher_name}</SettingsText>
          <Toggle
            checked={publisher.enabled || publisher.user_enabled === true}
            size='large'
          />
        </SettingsRow>
      ))}
    </div>
  )
}
