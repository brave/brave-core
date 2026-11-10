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
  url: string
}

export interface ShareCardImages {
  logo: string
  pattern: string
}

// Laid out in the design's 336x408 points (14:17) and drawn at 4x.
const WIDTH = 336
const HEIGHT = 408
const SCALE = 4
export const SHARE_CARD_WIDTH = WIDTH * SCALE
export const SHARE_CARD_HEIGHT = HEIGHT * SCALE

const PADDING = 24
const CONTENT_WIDTH = WIDTH - PADDING * 2
const TILE_SIZE = 72
const TILE_RADIUS = 16
// The release logo's artwork fills most, but not all, of its square.
const LOGO_SIZE = TILE_SIZE * 0.93
const HEADER_GAP = 16
const SECTION_GAP = 16
const PATTERN_SIZE = 486
const GRADIENT_ANGLE_DEG = 111.68
const STAT_VALUE_SIZE = 34
const STAT_LABEL_SIZE = 20
const STAT_LABEL_LINE = 26
const BUTTON_HEIGHT = 44
const BUTTON_PADDING = 16
const TEXT_SHADOW_OFFSET = 2

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

async function tryLoadImage(src: string) {
  try {
    return await loadImage(src)
  } catch {
    // The card still works without decoration.
    return undefined
  }
}

// Matches CSS linear-gradient(<angle>, ...) over the whole card.
function drawBackground(ctx: CanvasRenderingContext2D) {
  const angle = (GRADIENT_ANGLE_DEG * Math.PI) / 180
  const dx = Math.sin(angle)
  const dy = -Math.cos(angle)
  const half =
    (Math.abs(WIDTH * Math.sin(angle)) + Math.abs(HEIGHT * Math.cos(angle))) / 2
  const gradient = ctx.createLinearGradient(
    WIDTH / 2 - dx * half,
    HEIGHT / 2 - dy * half,
    WIDTH / 2 + dx * half,
    HEIGHT / 2 + dy * half,
  )
  gradient.addColorStop(0, getToken('color-primitive-brands-rorange-1'))
  gradient.addColorStop(0.5, getToken('color-primitive-brands-rorange-2'))
  gradient.addColorStop(1, getToken('color-primitive-brands-rorange-3'))
  ctx.fillStyle = gradient
  ctx.fillRect(0, 0, WIDTH, HEIGHT)
}

function setFont(
  ctx: CanvasRenderingContext2D,
  text: string,
  weight: number,
  size: number,
  family: string,
  maxWidth: number,
) {
  let fontSize = size
  ctx.font = `${weight} ${fontSize}px ${family}`
  while (ctx.measureText(text).width > maxWidth && fontSize > 8) {
    fontSize -= 1
    ctx.font = `${weight} ${fontSize}px ${family}`
  }
}

// Draws one line of text vertically centered in a line box.
function drawLine(
  ctx: CanvasRenderingContext2D,
  text: string,
  x: number,
  top: number,
  lineHeight: number,
) {
  ctx.fillText(text, x, top + lineHeight / 2)
}

function setTextShadow(ctx: CanvasRenderingContext2D, color: string) {
  // Shadow offsets are in device pixels, unaffected by the context's scale.
  ctx.shadowColor = color
  ctx.shadowOffsetX = TEXT_SHADOW_OFFSET * SCALE
  ctx.shadowOffsetY = TEXT_SHADOW_OFFSET * SCALE
  ctx.shadowBlur = 0
}

function clearShadow(ctx: CanvasRenderingContext2D) {
  ctx.shadowColor = 'transparent'
}

export async function renderShareCard(
  content: ShareCardContent,
  images: ShareCardImages,
): Promise<Blob> {
  const canvas = document.createElement('canvas')
  canvas.width = SHARE_CARD_WIDTH
  canvas.height = SHARE_CARD_HEIGHT
  const ctx = canvas.getContext('2d')!
  ctx.scale(SCALE, SCALE)

  const [logo, pattern] = await Promise.all([
    tryLoadImage(images.logo),
    tryLoadImage(images.pattern),
  ])

  drawBackground(ctx)
  if (pattern) {
    ctx.drawImage(
      pattern,
      (WIDTH - PATTERN_SIZE) / 2,
      (HEIGHT - PATTERN_SIZE) / 2 + 1,
      PATTERN_SIZE,
      PATTERN_SIZE,
    )
  }

  const family = getFontFamily()
  const white = getToken('color-white')
  const black = getToken('color-black')
  // Closest token to the design's #f01c00 shadow.
  const shadow = getToken('color-primitive-brands-rorange-3')
  ctx.textBaseline = 'middle'
  ctx.textAlign = 'left'

  // Header: logo tile, title and subtitle.
  ctx.fillStyle = white
  ctx.beginPath()
  ctx.roundRect(PADDING, PADDING, TILE_SIZE, TILE_SIZE, TILE_RADIUS)
  ctx.fill()
  if (logo) {
    const inset = (TILE_SIZE - LOGO_SIZE) / 2
    ctx.drawImage(logo, PADDING + inset, PADDING + inset, LOGO_SIZE, LOGO_SIZE)
  }

  setTextShadow(ctx, shadow)
  const headerX = PADDING + TILE_SIZE + HEADER_GAP
  const headerWidth = WIDTH - PADDING - headerX
  // Title (28) and subtitle (24) lines, centered against the tile.
  const headerTop = PADDING + (TILE_SIZE - 52) / 2
  setFont(ctx, content.title, 600, 22, family, headerWidth)
  drawLine(ctx, content.title, headerX, headerTop, 28)
  if (content.subtitle) {
    setFont(ctx, content.subtitle, 600, 16, family, headerWidth)
    drawLine(ctx, content.subtitle, headerX, headerTop + 28, 24)
  }

  // Stats: a heavy value with a label below.
  let y = PADDING + TILE_SIZE + SECTION_GAP
  for (const stat of content.stats) {
    setFont(ctx, stat.value, 800, STAT_VALUE_SIZE, family, CONTENT_WIDTH)
    drawLine(ctx, stat.value, PADDING, y, STAT_VALUE_SIZE)
    setFont(ctx, stat.label, 600, STAT_LABEL_SIZE, family, CONTENT_WIDTH)
    drawLine(ctx, stat.label, PADDING, y + STAT_VALUE_SIZE, STAT_LABEL_LINE)
    y += STAT_VALUE_SIZE + STAT_LABEL_LINE + SECTION_GAP
  }

  // Footer: a call to action pill, and the page address at the end.
  const footerTop = HEIGHT - PADDING - BUTTON_HEIGHT
  ctx.font = `400 14px ${family}`
  const urlWidth = ctx.measureText(content.url).width
  ctx.textAlign = 'right'
  drawLine(ctx, content.url, WIDTH - PADDING, footerTop, BUTTON_HEIGHT)

  clearShadow(ctx)
  ctx.textAlign = 'left'
  setFont(
    ctx,
    content.cta,
    600,
    14,
    family,
    CONTENT_WIDTH - urlWidth - HEADER_GAP - BUTTON_PADDING * 2,
  )
  const buttonWidth = ctx.measureText(content.cta).width + BUTTON_PADDING * 2
  ctx.fillStyle = black
  ctx.beginPath()
  ctx.roundRect(
    PADDING,
    footerTop,
    buttonWidth,
    BUTTON_HEIGHT,
    BUTTON_HEIGHT / 2,
  )
  ctx.fill()
  ctx.fillStyle = white
  drawLine(ctx, content.cta, PADDING + BUTTON_PADDING, footerTop, BUTTON_HEIGHT)

  return new Promise<Blob>((resolve, reject) => {
    canvas.toBlob(
      (blob) => (blob ? resolve(blob) : reject(new Error('toBlob failed'))),
      'image/png',
    )
  })
}
