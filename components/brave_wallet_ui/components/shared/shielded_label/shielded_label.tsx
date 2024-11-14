// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Label from '@brave/leo/react/label'
import Icon from '@brave/leo/react/icon'

// Utils
import { getLocale } from '../../../../common/locale'

export const ShieldedLabel = () => {
  return (
    <Label color='neutral'>
      <div slot='icon-before'>
        <Icon name='shield-done' />
      </div>
      {getLocale('braveWalletShielded')}
    </Label>
  )
}
