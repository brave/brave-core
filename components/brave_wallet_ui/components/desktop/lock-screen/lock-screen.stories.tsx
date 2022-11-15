// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import LockScreen from '.'

export const _LockScreen: React.FC = () => {
  return (
    <LockScreen
      disabled={false}
      hasPasswordError={false}
      onPasswordChanged={() => null}
      onShowRestore={() => null}
      onSubmit={() => null}
    />
  )
}

export default _LockScreen
