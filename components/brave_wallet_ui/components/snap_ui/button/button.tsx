// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import Button from '@brave/leo/react/button'

import type { SnapUiButtonProps } from '../snap_ui_sdk_props'
import type { SnapUiOnButton } from '../snap_ui_wallet_renderer.types'
import { isSnapUiElement, normalizeChildList } from '../snap_ui_utils'

function buttonLabelFromChildren(children: unknown): string {
  const parts = normalizeChildList(children)
  const strings: string[] = []
  for (const p of parts) {
    if (typeof p === 'string' || typeof p === 'number') {
      strings.push(String(p))
    }
    if (isSnapUiElement(p) && p.type === 'Icon') {
      strings.push('\u25cf')
    }
  }
  return strings.join(' ').trim()
}

export function renderSnapUiButton(
  props: SnapUiButtonProps,
  reactKey: string,
  onSnapButton?: SnapUiOnButton,
): React.ReactNode {
  const variant = props.variant === 'destructive' ? 'destructive' : 'primary'
  const size = props.size === 'sm' ? 'small' : 'medium'
  const label = buttonLabelFromChildren(props.children)
  const name = props.name
  const snapButtonType = props.type === 'submit' ? 'submit' : 'button'
  const disabled = props.disabled === true
  const loading = props.loading === true
  const kind = variant === 'destructive' ? 'outline' : 'filled'
  return (
    <Button
      key={reactKey}
      kind={kind}
      size={size}
      isDisabled={disabled}
      isLoading={loading}
      onClick={() => onSnapButton?.({ name, snapButtonType })}
    >
      {label || '\u00a0'}
    </Button>
  )
}
