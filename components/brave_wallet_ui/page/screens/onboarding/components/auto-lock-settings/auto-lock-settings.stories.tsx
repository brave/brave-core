// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import WalletPageStory from '../../../../../stories/wrappers/wallet-page-story-wrapper'
import { AutoLockSettings } from './auto-lock-settings'
import { Row } from '../../../../../components/shared/style'
import { autoLockOptions } from '../../../../../options/auto-lock-options'

export const _AutoLockSettings = () => {
  const [value, setValue] = React.useState(autoLockOptions[0].value)

  return (
    <WalletPageStory>
      <Row
        justifyContent='center'
        alignItems='center'
        padding='0 23px 0'
      >
        <AutoLockSettings
          options={autoLockOptions}
          value={value}
          onChange={setValue}
        />
      </Row>
    </WalletPageStory>
  )
}

_AutoLockSettings.story = {
  name: 'Auto lock settings'
}

export default _AutoLockSettings
