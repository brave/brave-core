// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// Renders the shareable card entirely on-device. It contains only aggregate
// counts and the install month, never sites or browsing history.

export interface ShareCardContent {
  title: string
  subtitle: string
  stats: Array<{ value: string; label: string }>
  cta: string
}

// 4:5 portrait works well on both X and Reddit.
const WIDTH = 1080
const HEIGHT = 1350
const PADDING = 96
const LOGO_SIZE = 96
const LOGO_PADDING = 24

function getToken(name: string) {
  return getComputedStyle(document.documentElement)
    .getPropertyValue(`--leo-${name}`)
    .trim()
}

function getFontFamily() {
  return getToken('font-heading-h1').replace(/^.*?\d+px\/\d+px\s*/, '')
}

function loadImage(src: string) {
  return new Promise<HTMLImageElement>((resolve, reject) => {
    const image = new Image()
    image.onload = () => resolve(image)
    image.onerror = reject
    image.src = src
  })
}

function drawBackground(ctx: CanvasRenderingContext2D) {
  const gradient = ctx.createLinearGradient(0, 0, 0, HEIGHT)
  gradient.addColorStop(0, getToken('color-primitive-brands-rorange-1'))
  gradient.addColorStop(1, getToken('color-primitive-brands-rorange-3'))
  ctx.fillStyle = gradient
  ctx.fillRect(0, 0, WIDTH, HEIGHT)
}

// Shrinks the font until `text` fits in `maxWidth`.
function fitFont(
  ctx: CanvasRenderingContext2D,
  text: string,
  weight: number,
  size: number,
  family: string,
  maxWidth: number,
) {
  let fontSize = size
  do {
    ctx.font = `${weight} ${fontSize}px ${family}`
    fontSize -= 4
  } while (ctx.measureText(text).width > maxWidth && fontSize > 24)
}

function roundRect(
  ctx: CanvasRenderingContext2D,
  x: number,
  y: number,
  w: number,
  h: number,
  r: number,
) {
  ctx.beginPath()
  ctx.moveTo(x + r, y)
  ctx.arcTo(x + w, y, x + w, y + h, r)
  ctx.arcTo(x + w, y + h, x, y + h, r)
  ctx.arcTo(x, y + h, x, y, r)
  ctx.arcTo(x, y, x + w, y, r)
  ctx.closePath()
}

export async function renderShareCard(
  content: ShareCardContent,
  logoUrl: string,
): Promise<Blob> {
  const canvas = document.createElement('canvas')
  canvas.width = WIDTH
  canvas.height = HEIGHT
  const ctx = canvas.getContext('2d')!
  drawBackground(ctx)

  const family = getFontFamily()
  const white = getToken('color-white')
  const contentWidth = WIDTH - PADDING * 2
  ctx.textBaseline = 'alphabetic'

  // The tile keeps the colour logo visible on the orange.
  const tileSize = LOGO_SIZE + LOGO_PADDING * 2
  ctx.fillStyle = white
  roundRect(ctx, PADDING, PADDING, tileSize, tileSize, 32)
  ctx.fill()
  try {
    const logo = await loadImage(logoUrl)
    ctx.drawImage(
      logo,
      PADDING + LOGO_PADDING,
      PADDING + LOGO_PADDING,
      LOGO_SIZE,
      LOGO_SIZE,
    )
  } catch {
    // The card still works without the logo.
  }
  const headerX = PADDING + tileSize + 40
  ctx.fillStyle = white
  fitFont(ctx, content.title, 600, 60, family, WIDTH - headerX - PADDING)
  ctx.fillText(content.title, headerX, PADDING + 66)
  ctx.font = `400 36px ${family}`
  ctx.fillText(content.subtitle, headerX, PADDING + 120)

  // Stats, evenly spaced between the header and the footer.
  const statsTop = PADDING + tileSize + 96
  const statsBottom = HEIGHT - PADDING - 150
  const rowHeight = (statsBottom - statsTop) / content.stats.length
  content.stats.forEach((stat, i) => {
    const y = statsTop + rowHeight * i
    fitFont(ctx, stat.value, 600, 136, family, contentWidth)
    ctx.fillText(stat.value, PADDING, y + 130)
    fitFont(ctx, stat.label, 400, 44, family, contentWidth)
    ctx.fillText(stat.label, PADDING, y + 190)
  })

  // Footer pill.
  ctx.font = `600 40px ${family}`
  const ctaWidth = ctx.measureText(content.cta).width + 96
  const ctaHeight = 96
  const ctaY = HEIGHT - PADDING - ctaHeight
  ctx.fillStyle = white
  roundRect(ctx, PADDING, ctaY, ctaWidth, ctaHeight, ctaHeight / 2)
  ctx.fill()
  ctx.fillStyle = getToken('color-primitive-neutral-10')
  ctx.fillText(content.cta, PADDING + 48, ctaY + 62)

  return new Promise<Blob>((resolve, reject) => {
    canvas.toBlob(
      (blob) => (blob ? resolve(blob) : reject(new Error('toBlob failed'))),
      'image/png',
    )
  })
}
