// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

import type { SnapUiHeadingProps } from '../snap_ui_sdk_props'
import { normalizeChildList } from '../snap_ui_utils'

type SnapHeadingSize = 'sm' | 'md' | 'lg'

function snapHeadingFontSize(size: SnapHeadingSize): string {
  if (size === 'lg') {
    return '22px'
  }
  if (size === 'md') {
    return '18px'
  }
  return '16px'
}

const SnapHeadingRoot = styled.h2<{ $size: SnapHeadingSize }>`
  margin: 0;
  font-family: Poppins, sans-serif;
  font-weight: 600;
  color: ${leo.color.text.primary};
  line-height: 1.3;
  font-size: ${(p) => snapHeadingFontSize(p.$size)};
`

function stringFromSnapChildren(children: unknown): string {
  const parts = normalizeChildList(children)
  const strings: string[] = []
  for (const p of parts) {
    if (typeof p === 'string' || typeof p === 'number') {
      strings.push(String(p))
    }
  }
  return strings.join('')
}

export function renderSnapUiHeading(
  props: SnapUiHeadingProps,
  reactKey: string,
): React.ReactNode {
  const size: SnapHeadingSize = props.size ?? 'sm'
  const label = stringFromSnapChildren(props.children)
  return (
    <SnapHeadingRoot
      key={reactKey}
      $size={size}
    >
      {label}
    </SnapHeadingRoot>
  )
}
