// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as leo from '@brave/leo/tokens/css/variables'

export function renderSnapUiUnsupported(
  type: string,
  reactKey: string,
): React.ReactNode {
  return (
    <div
      key={reactKey}
      style={{
        fontSize: 12,
        color: leo.color.text.tertiary,
        fontFamily: 'monospace',
      }}
    >
      Unsupported Snap UI type: {type}
    </div>
  )
}
