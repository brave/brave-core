// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import type * as React from 'react'

import type { SnapUiElement } from './snap_ui_types'

export interface SnapUiWalletRendererProps {
  /** Root Snaps UI element (e.g. from snap_createInterface params.ui). */
  root: SnapUiElement
  /** Fired when the user activates a Snap `Button`. */
  onSnapButton?: (detail: {
    name: string | undefined
    snapButtonType: 'button' | 'submit'
  }) => void
  /** Fired when the user edits a Snap `Input` (controlled locally until the host updates the tree). */
  onSnapInput?: (detail: { name: string; value: string }) => void
}

export type SnapUiOnButton = NonNullable<
  SnapUiWalletRendererProps['onSnapButton']
>

export type SnapUiOnInput = NonNullable<
  SnapUiWalletRendererProps['onSnapInput']
>

export type SnapUiRenderChildFn = (
  child: unknown,
  index: number,
  keyPrefix: string,
) => React.ReactNode
