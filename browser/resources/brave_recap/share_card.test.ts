// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { SHARE_CARD_WIDTH, renderShareCard } from './share_card'

const WIDTH = SHARE_CARD_WIDTH / 4

// Records where text and boxes are drawn, in card points.
async function drawCard(direction: 'ltr' | 'rtl') {
  const text: Array<{ text: string; x: number; align: string }> = []
  const boxes: Array<{ x: number; width: number }> = []
  const state: Record<string | symbol, unknown> = {}
  const ctx = new Proxy(state, {
    get(target, name) {
      switch (name) {
        case 'fillText':
          return (value: string, x: number) =>
            text.push({ text: value, x, align: String(target.textAlign) })
        case 'roundRect':
          return (x: number, _y: number, width: number) =>
            boxes.push({ x, width })
        case 'measureText':
          return (value: string) => ({ width: value.length * 5 })
        case 'createLinearGradient':
          return () => ({ addColorStop: () => {} })
      }
      return name in target ? target[name] : () => {}
    },
  })
  jest
    .spyOn(HTMLCanvasElement.prototype, 'getContext')
    .mockReturnValue(ctx as unknown as CanvasRenderingContext2D)
  jest
    .spyOn(HTMLCanvasElement.prototype, 'toBlob')
    .mockImplementation((callback) => callback(new Blob()))
  // jsdom never loads images, so fail them; the card is drawn without them.
  jest.spyOn(window, 'Image').mockImplementation(() => {
    const image = {} as HTMLImageElement
    Object.defineProperty(image, 'src', {
      set: () => queueMicrotask(() => image.onerror?.('' as any)),
    })
    return image
  })
  jest.spyOn(window, 'getComputedStyle').mockReturnValue({
    direction,
    getPropertyValue: () => '',
  } as unknown as CSSStyleDeclaration)

  await renderShareCard(
    {
      title: 'title',
      subtitle: 'subtitle',
      stats: [{ value: '1,000', label: 'blocked' }],
      cta: 'cta',
      url: 'url',
    },
    { logo: 'logo.svg', pattern: 'pattern.webp' },
  )
  jest.restoreAllMocks()
  return { text, boxes, direction: state.direction }
}

describe('share card', () => {
  it('mirrors its layout in right-to-left layouts', async () => {
    const ltr = await drawCard('ltr')
    const rtl = await drawCard('rtl')

    expect(ltr.direction).toBe('ltr')
    expect(rtl.direction).toBe('rtl')
    expect(rtl.text).toEqual(
      ltr.text.map((line) => ({ ...line, x: WIDTH - line.x })),
    )
    expect(rtl.boxes).toEqual(
      ltr.boxes.map((box) => ({ ...box, x: WIDTH - box.x - box.width })),
    )
  })
})
