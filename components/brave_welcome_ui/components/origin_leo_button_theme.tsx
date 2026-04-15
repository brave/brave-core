// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

interface OriginLeoButtonThemeProps {
  children: React.ReactNode
}

/**
 * Brave Origin onboarding uses light imagery while standard Brave may follow
 * system dark mode. Leo buttons respect `data-theme` on an ancestor; this
 * wrapper forces dark button styling for Origin builds only.
 */
export default function OriginLeoButtonTheme (props: OriginLeoButtonThemeProps) {
  let themed: React.ReactNode
  // <if expr="is_brave_origin_branded">
  themed = (
    <span data-theme='dark' style={{ display: 'contents' }}>
      {props.children}
    </span>
  )
  // <else>
  themed = <>{props.children}</>
  // </if>
  return themed
}
