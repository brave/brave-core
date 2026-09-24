// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Checkbox from '@brave/leo/react/checkbox'
import type { Element as HastElement, Root } from 'hast'
import * as React from 'react'
import { visit } from 'unist-util-visit'

const TASK_INDEX_ATTR = 'data-task-index'

// Rehype plugin: assigns a sequential `taskIndex` to each task-list checkbox
// <input> in document order. The renderer surfaces this as a
// `data-task-index` attribute on the leo-checkbox so the click handler can
// identify which checkbox was toggled without tracking offsets across
// re-renders. The same order is produced by
// findTaskCheckboxBracketOffsets() over the source string, so the index
// round-trips back to a position in the markdown.
export const rehypeTaskCheckboxIndex = () => (tree: Root) => {
  let index = 0
  visit(tree, 'element', (node: HastElement) => {
    if (node.tagName !== 'input') return
    const props = node.properties as { type?: string; taskIndex?: number }
    if (props?.type !== 'checkbox') return
    props.taskIndex = index++
  })
}

export type CheckboxToggleHandler = (index: number, checked: boolean) => void

export const createLiClickHandler =
  (onToggle?: CheckboxToggleHandler): React.MouseEventHandler =>
  (e) => {
    // The leo-checkbox is unlabeled, so toggle from the surrounding <li>.
    // preventDefault stops the click from also reaching the checkbox itself,
    // which would immediately undo our toggle.
    const checkBox = e.currentTarget?.querySelector<
      HTMLElement & { checked: boolean }
    >('leo-checkbox')
    if (!checkBox) return

    e.preventDefault()
    const newChecked = !checkBox.checked
    checkBox.checked = newChecked

    if (!onToggle) return
    const attr = checkBox.getAttribute(TASK_INDEX_ATTR)
    if (attr === null) return
    const index = Number(attr)
    if (!Number.isFinite(index)) return
    onToggle(index, newChecked)
  }

export const checkboxRenderer = (props: any) => {
  if (props.type !== 'checkbox') return null
  const taskIndex = props.node?.properties?.taskIndex
  const extra =
    typeof taskIndex === 'number' ? { [TASK_INDEX_ATTR]: taskIndex } : {}
  return (
    <Checkbox
      checked={!!props.checked}
      {...extra}
    />
  )
}
