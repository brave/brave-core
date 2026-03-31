// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import styled from 'styled-components'
import * as leo from '@brave/leo/tokens/css/variables'

import type { SnapUiTextProps } from '../snap_ui_sdk_props'
import { isSnapUiElement, normalizeChildList } from '../snap_ui_utils'

type SnapTextAlignment = 'start' | 'center' | 'end'
type SnapTextBodySize = 'sm' | 'md'
type SnapTextFontWeightKey = 'regular' | 'medium' | 'bold'

type TextColorKey =
  | 'default'
  | 'alternative'
  | 'muted'
  | 'error'
  | 'success'
  | 'warning'

function snapTextAlignCss(alignment: SnapTextAlignment): string {
  if (alignment === 'center') {
    return 'center'
  }
  if (alignment === 'end') {
    return 'right'
  }
  return 'left'
}

function snapTextFontPx(size: SnapTextBodySize): string {
  return size === 'sm' ? '12px' : '14px'
}

function snapTextLineHeightPx(size: SnapTextBodySize): string {
  return size === 'sm' ? '18px' : '20px'
}

function snapTextFontWeight(w: SnapTextFontWeightKey): number {
  if (w === 'bold') {
    return 600
  }
  if (w === 'medium') {
    return 500
  }
  return 400
}

const TEXT_COLOR: Record<TextColorKey, string> = {
  default: leo.color.text.primary,
  alternative: leo.color.text.secondary,
  muted: leo.color.text.tertiary,
  error: leo.color.systemfeedback.errorText,
  success: leo.color.systemfeedback.successText,
  warning: leo.color.systemfeedback.warningText,
}

const SnapTextRoot = styled.span<{
  $alignment: SnapTextAlignment
  $color: string
  $size: SnapTextBodySize
  $fontWeight: SnapTextFontWeightKey
}>`
  display: block;
  text-align: ${(p) => snapTextAlignCss(p.$alignment)};
  font-family: Poppins, sans-serif;
  font-size: ${(p) => snapTextFontPx(p.$size)};
  line-height: ${(p) => snapTextLineHeightPx(p.$size)};
  font-weight: ${(p) => snapTextFontWeight(p.$fontWeight)};
  color: ${(p) => p.$color};
  white-space: pre-wrap;
`

function renderTextInner(
  children: unknown,
  keyBase: string,
): React.ReactNode[] {
  const parts = normalizeChildList(children)
  const nodes: React.ReactNode[] = []
  let i = 0
  for (const part of parts) {
    const key = `${keyBase}-${i++}`
    if (part === null || part === false || typeof part === 'boolean') {
      continue
    }
    if (typeof part === 'string' || typeof part === 'number') {
      nodes.push(<React.Fragment key={key}>{String(part)}</React.Fragment>)
      continue
    }
    if (!isSnapUiElement(part)) {
      continue
    }
    if (part.type === 'Bold') {
      nodes.push(
        <strong key={key}>
          {renderTextInner(part.props.children, `${key}-b`)}
        </strong>,
      )
      continue
    }
    if (part.type === 'Italic') {
      nodes.push(
        <em key={key}>{renderTextInner(part.props.children, `${key}-i`)}</em>,
      )
      continue
    }
    nodes.push(
      <span
        key={key}
        style={{ opacity: 0.7 }}
      >
        [unsupported:{part.type}]
      </span>,
    )
  }
  return nodes
}

function textColorKeyFromSnap(color: SnapUiTextProps['color']): TextColorKey {
  if (
    color === 'alternative'
    || color === 'muted'
    || color === 'error'
    || color === 'success'
    || color === 'warning'
  ) {
    return color
  }
  return 'default'
}

export function renderSnapUiText(
  props: SnapUiTextProps,
  reactKey: string,
): React.ReactNode {
  const alignment: SnapTextAlignment = props.alignment ?? 'start'
  const colorKey = textColorKeyFromSnap(props.color)
  const size: SnapTextBodySize = props.size === 'sm' ? 'sm' : 'md'
  const fontWeight: SnapTextFontWeightKey = props.fontWeight ?? 'regular'
  return (
    <SnapTextRoot
      key={reactKey}
      $alignment={alignment}
      $color={TEXT_COLOR[colorKey]}
      $size={size}
      $fontWeight={fontWeight}
    >
      {renderTextInner(props.children, reactKey)}
    </SnapTextRoot>
  )
}
