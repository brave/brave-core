// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import {
  SHARE_CARD_WIDTH,
  ShareCardContent,
  renderShareCard,
} from './share_card'

const WIDTH = SHARE_CARD_WIDTH / 4

const CONTENT: ShareCardContent = {
  title: 'title',
  subtitle: 'subtitle',
  stats: [{ value: '1,000', label: 'blocked' }],
  cta: 'cta',
  url: 'url',
}

// Records where text and boxes are drawn, in card points.
async function drawCard(
  direction: 'ltr' | 'rtl',
  content: Partial<ShareCardContent> = {},
) {
  const text: Array<{
    text: string
    x: number
    y: number
    maxWidth?: number
    align: string
  }> = []
  const boxes: Array<{ x: number; width: number }> = []
  const state: Record<string | symbol, unknown> = {}
  const ctx = new Proxy(state, {
    get(target, name) {
      switch (name) {
        case 'fillText':
          return (value: string, x: number, y: number, maxWidth?: number) =>
            text.push({
              text: value,
              x,
              y,
              maxWidth,
              align: String(target.textAlign),
            })
        case 'roundRect':
          return (x: number, _y: number, width: number) =>
            boxes.push({ x, width })
        case 'measureText':
          return (value: string) => ({ width: value.length * 5 })
        case 'createLinearGradient':
          return () => ({
            // Like canvas, which throws on an unparsable color.
            addColorStop: (_offset: number, color: string) => {
              if (!color) {
                throw new SyntaxError('empty color')
              }
            },
          })
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
    { ...CONTENT, ...content },
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

  it('centers a lone title against the logo tile', async () => {
    const { text, boxes } = await drawCard('ltr', { subtitle: '' })
    const tile = boxes[0]
    const title = text.find((line) => line.text === 'title')!
    // The tile is square and sits at the card's padding.
    expect(title.y).toBe(tile.x + tile.width / 2)
  })

  it('keeps a long call to action clear of the address', async () => {
    const cta = 'x'.repeat(200)
    const { text, boxes } = await drawCard('ltr', { cta })
    const pill = boxes[1]
    const address = text.find((line) => line.text === 'url')!
    const label = text.find((line) => line.text === cta)!
    // The address is end-aligned, so it starts its own width before `x`.
    expect(pill.x + pill.width).toBeLessThan(address.x - 'url'.length * 5)
    expect(label.maxWidth).toBeLessThan(pill.width)
  })
})
