// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { SKIP, visit } from 'unist-util-visit'
import type { Root, Text, InlineCode } from 'mdast'
import type { Parent } from 'unist'
import Tooltip from '@brave/leo/react/tooltip'
import Icon from '@brave/leo/react/icon'
import styles from './style.module.scss'

// Matches hex (#rgb, #rrggbb, #rgba, #rrggbbaa), rgb/rgba, hsl/hsla.
// Hex uses a negative lookahead to avoid partial matches (e.g. 7-digit strings).
const COLOR_REGEX = new RegExp(
  [
    '#(?:[0-9a-fA-F]{8}|[0-9a-fA-F]{6}|[0-9a-fA-F]{4}|[0-9a-fA-F]{3})(?![0-9a-fA-F])',
    'rgba?\\(\\s*\\d{1,3}%?\\s*,\\s*\\d{1,3}%?\\s*,\\s*\\d{1,3}%?(?:\\s*,\\s*[\\d.]+%?)?\\s*\\)',
    'hsla?\\(\\s*[\\d.]+(?:deg|rad|turn|grad)?\\s*,\\s*[\\d.]+%\\s*,\\s*[\\d.]+%(?:\\s*,\\s*[\\d.]+%?)?\\s*\\)',
  ].join('|'),
  'gi',
)

function hueToRgb(p: number, q: number, t: number): number {
  const tt = t < 0 ? t + 1 : t > 1 ? t - 1 : t
  if (tt < 1 / 6) return p + (q - p) * 6 * tt
  if (tt < 1 / 2) return q
  if (tt < 2 / 3) return p + (q - p) * (2 / 3 - tt) * 6
  return p
}

function parseColorToRgb(color: string): [number, number, number] | null {
  const s = color.trim()

  const hexMatch = s.match(/^#([0-9a-fA-F]+)$/)
  if (hexMatch) {
    let h = hexMatch[1]
    if (h.length === 3 || h.length === 4)
      h = h[0] + h[0] + h[1] + h[1] + h[2] + h[2]
    return [
      parseInt(h.slice(0, 2), 16),
      parseInt(h.slice(2, 4), 16),
      parseInt(h.slice(4, 6), 16),
    ]
  }

  const rgbMatch = s.match(
    /rgba?\(\s*([\d.]+)(%?)\s*,\s*([\d.]+)(%?)\s*,\s*([\d.]+)(%?)/,
  )
  if (rgbMatch) {
    const ch = (v: string, pct: string) =>
      pct ? parseFloat(v) * 2.55 : parseFloat(v)
    return [
      ch(rgbMatch[1], rgbMatch[2]),
      ch(rgbMatch[3], rgbMatch[4]),
      ch(rgbMatch[5], rgbMatch[6]),
    ]
  }

  const hslMatch = s.match(
    /hsla?\(\s*([\d.]+)(deg|rad|turn|grad)?\s*,\s*([\d.]+)%\s*,\s*([\d.]+)%/,
  )
  if (hslMatch) {
    const unitMultiplier: Record<string, number> = {
      deg: 1 / 360,
      rad: 1 / (2 * Math.PI),
      turn: 1,
      grad: 1 / 400,
    }
    const h =
      parseFloat(hslMatch[1])
      * (unitMultiplier[hslMatch[2] ?? 'deg'] ?? 1 / 360)
    const sl = parseFloat(hslMatch[3]) / 100
    const l = parseFloat(hslMatch[4]) / 100
    if (sl === 0) {
      const v = Math.round(l * 255)
      return [v, v, v]
    }
    const q = l < 0.5 ? l * (1 + sl) : l + sl - l * sl
    const p = 2 * l - q
    return [
      Math.round(hueToRgb(p, q, h + 1 / 3) * 255),
      Math.round(hueToRgb(p, q, h) * 255),
      Math.round(hueToRgb(p, q, h - 1 / 3) * 255),
    ]
  }

  return null
}

// Returns '#ffffff' or '#000000' — whichever has higher WCAG contrast against the color.
function contrastColor(color: string): string {
  const rgb = parseColorToRgb(color)
  if (!rgb) return '#000000'
  const [r, g, b] = rgb.map((c) => {
    const n = c / 255
    return n <= 0.04045 ? n / 12.92 : Math.pow((n + 0.055) / 1.055, 2.4)
  })
  const L = 0.2126 * r + 0.7152 * g + 0.0722 * b
  return L < 0.179 ? '#ffffff' : '#000000'
}

export function remarkColor() {
  return (tree: Root) => {
    visit(
      tree,
      'text',
      (node: Text, index: number | null, parent: Parent | null | undefined) => {
        if (!parent || index === null || index === undefined) return

        const text = node.value
        const parts: any[] = []
        let lastIndex = 0
        let match: RegExpExecArray | null

        COLOR_REGEX.lastIndex = 0
        while ((match = COLOR_REGEX.exec(text)) !== null) {
          if (match.index > lastIndex) {
            parts.push({
              type: 'text',
              value: text.slice(lastIndex, match.index),
            })
          }
          const color = match[0]
          parts.push({
            type: 'colorchip',
            value: color,
            data: {
              hName: 'colorchip',
              hProperties: { color },
            },
            children: [{ type: 'text', value: color }],
          })
          lastIndex = match.index + color.length
        }

        if (parts.length === 0) return

        if (lastIndex < text.length) {
          parts.push({ type: 'text', value: text.slice(lastIndex) })
        }

        ;(parent.children as any[]).splice(index, 1, ...parts)
        return [SKIP, index + parts.length]
      },
    )

    // Replace inlineCode nodes whose entire value is a color (e.g. `#ff0000`).
    visit(
      tree,
      'inlineCode',
      (
        node: InlineCode,
        index: number | null,
        parent: Parent | null | undefined,
      ) => {
        if (!parent || index === null || index === undefined) return

        COLOR_REGEX.lastIndex = 0
        const match = COLOR_REGEX.exec(node.value)
        if (
          !match
          || match.index !== 0
          || match[0].length !== node.value.length
        )
          return

        const color = match[0]
        ;(parent.children as any[]).splice(index, 1, {
          type: 'colorchip',
          value: color,
          data: {
            hName: 'colorchip',
            hProperties: { color },
          },
          children: [{ type: 'text', value: color }],
        })
      },
    )
  }
}

export function ColorChip(props: {
  color?: string
  children?: React.ReactNode
}) {
  const [copied, setCopied] = React.useState(false)
  const textColor = props.color ? contrastColor(props.color) : undefined

  const handleClick = React.useCallback(() => {
    if (!props.color) return
    navigator.clipboard.writeText(props.color)
    setCopied(true)
    setTimeout(() => setCopied(false), 500)
  }, [props.color])

  return (
    <Tooltip
      mode='mini'
      visible={copied}
      text='Copied'
    >
      <span
        className={styles.colorChip}
        style={{ backgroundColor: props.color, color: textColor }}
        onClick={handleClick}
        role='button'
      >
        {props.children}
        <Icon
          name='copy'
          className={styles.copyIcon}
        />
      </span>
    </Tooltip>
  )
}
