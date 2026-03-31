// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'

import type { SnapUiBoxProps } from '../snap_ui_sdk_props'
import type { SnapUiRenderChildFn } from '../snap_ui_wallet_renderer.types'
import { normalizeChildList } from '../snap_ui_utils'

function mapFlexJustify(alignment: string): string {
  switch (alignment) {
    case 'center':
      return 'center'
    case 'end':
      return 'flex-end'
    case 'space-between':
      return 'space-between'
    case 'space-around':
      return 'space-around'
    default:
      return 'flex-start'
  }
}

function mapFlexAlign(alignment: string): string {
  switch (alignment) {
    case 'center':
      return 'center'
    case 'end':
      return 'flex-end'
    default:
      return 'flex-start'
  }
}

const SnapBoxRoot = styled.div<{
  $direction: 'vertical' | 'horizontal'
  $mainAlign: string
  $crossAlign: string
  $center?: boolean
}>`
  display: flex;
  flex-direction: ${(p) => (p.$direction === 'horizontal' ? 'row' : 'column')};
  justify-content: ${(p) =>
    p.$center ? 'center' : mapFlexJustify(p.$mainAlign)};
  align-items: ${(p) => (p.$center ? 'center' : mapFlexAlign(p.$crossAlign))};
  gap: 12px;
  width: 100%;
  box-sizing: border-box;
`

export function renderSnapUiBox(
  props: SnapUiBoxProps,
  reactKey: string,
  renderChild: SnapUiRenderChildFn,
): React.ReactNode {
  const direction = props.direction === 'horizontal' ? 'horizontal' : 'vertical'
  const mainAlign = props.alignment ?? 'start'
  const crossAlign = props.crossAlignment ?? 'start'
  const center = props.center === true
  const children = normalizeChildList(props.children)
  return (
    <SnapBoxRoot
      key={reactKey}
      $direction={direction}
      $mainAlign={mainAlign}
      $crossAlign={crossAlign}
      $center={center}
    >
      {children.map((ch, idx) => renderChild(ch, idx, `${reactKey}-c`))}
    </SnapBoxRoot>
  )
}
