// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

/**
 * Renders Snaps custom UI element trees using Brave Wallet / Leo styling so
 * embedded Snap flows match the rest of the wallet.
 */

import * as React from 'react'

// Props
import type {
  SnapUiBoxProps,
  SnapUiButtonProps,
  SnapUiHeadingProps,
  SnapUiInputProps,
  SnapUiTextProps,
} from './snap_ui_sdk_props'

// Types
import { type SnapUiElement } from './snap_ui_types'
import type { SnapUiWalletRendererProps } from './snap_ui_wallet_renderer.types'

// Utils
import { reactKeyFor, isSnapUiElement } from './snap_ui_utils'

// Elements
import { renderSnapUiBox } from './box/box'
import { renderSnapUiButton } from './button/button'
import { renderSnapUiHeading } from './heading/heading'
import { renderSnapUiInput } from './input/input'
import { renderSnapUiText } from './text/text'
import { renderSnapUiUnsupported } from './unsupported/unsupported'

export type { SnapUiWalletRendererProps } from './snap_ui_wallet_renderer.types'

function renderChild(
  child: unknown,
  index: number,
  onSnapButton: SnapUiWalletRendererProps['onSnapButton'],
  onSnapInput: SnapUiWalletRendererProps['onSnapInput'],
  keyPrefix: string,
): React.ReactNode {
  if (child === null || child === false || typeof child === 'boolean') {
    return null
  }
  if (typeof child === 'string' || typeof child === 'number') {
    return <span key={`${keyPrefix}-${index}`}>{String(child)}</span>
  }
  if (!isSnapUiElement(child)) {
    return null
  }
  const pathKey = `${keyPrefix}-${index}`
  return renderNode(
    child,
    onSnapButton,
    onSnapInput,
    pathReactKey(pathKey, child),
  )
}

function pathReactKey(pathKey: string, element: SnapUiElement): string {
  const snapKey = element.key
  if (snapKey !== null && snapKey !== undefined && String(snapKey) !== '') {
    return `${pathKey}__${String(snapKey)}`
  }
  return pathKey
}

function renderNode(
  element: SnapUiElement,
  onSnapButton: SnapUiWalletRendererProps['onSnapButton'],
  onSnapInput: SnapUiWalletRendererProps['onSnapInput'],
  reactKey: string,
): React.ReactNode {
  const { type, props } = element

  if (type === 'Box') {
    return renderSnapUiBox(
      props as SnapUiBoxProps,
      reactKey,
      (child, index, keyPrefix) =>
        renderChild(child, index, onSnapButton, onSnapInput, keyPrefix),
    )
  }
  if (type === 'Heading') {
    return renderSnapUiHeading(props as SnapUiHeadingProps, reactKey)
  }
  if (type === 'Text') {
    return renderSnapUiText(props as SnapUiTextProps, reactKey)
  }
  if (type === 'Button') {
    return renderSnapUiButton(
      props as SnapUiButtonProps,
      reactKey,
      onSnapButton,
    )
  }
  if (type === 'Input') {
    return renderSnapUiInput(props as SnapUiInputProps, reactKey, onSnapInput)
  }
  return renderSnapUiUnsupported(type, reactKey)
}

export const SnapUiWalletRenderer: React.FC<SnapUiWalletRendererProps> = ({
  root,
  onSnapButton,
  onSnapInput,
}) => {
  return <>{renderNode(root, onSnapButton, onSnapInput, reactKeyFor(root))}</>
}
