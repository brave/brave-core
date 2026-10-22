// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

const TURN_MS = 900
const SCREEN_POLL_MS = 600

const deck = document.getElementById('deck')!
const slides = [...deck.querySelectorAll<HTMLElement>('.slide')]
const watermark = document.querySelector<HTMLElement>('.watermark')!
const progress = document.getElementById('progress')!
const counter = document.getElementById('counter')!

let current = -1
let turning = false

function fitToWindow() {
  const scale = Math.min(innerWidth / 1700, innerHeight / 960)
  deck.style.setProperty('--scale', String(scale))
}

function stagger(slide: HTMLElement) {
  slide.querySelectorAll<HTMLElement | SVGElement>('.reveal, .draw').forEach(
    (el, i) => el.style.setProperty('--i', String(i)),
  )
}

function show(index: number) {
  index = Math.max(0, Math.min(slides.length - 1, index))
  if (index === current || turning) {
    return
  }
  const previous = slides[current]
  const next = slides[index]
  const forward = index > current

  next.classList.remove('in')
  next.classList.add('active')
  if (previous) {
    turning = true
    // Forward lifts the old page away; backward lays the new one back down.
    const turned = forward ? previous : next
    turned.classList.add(forward ? 'turn-out' : 'turn-in')
    setTimeout(() => {
      turned.classList.remove('turn-out', 'turn-in')
      previous.classList.remove('active', 'in')
      next.classList.add('in')
      turning = false
    }, TURN_MS)
  } else {
    next.classList.add('in')
  }

  current = index
  history.replaceState(null, '', `#${index + 1}`)
  progress.style.setProperty('--progress', String(index / (slides.length - 1)))
  counter.textContent = `${index + 1} / ${slides.length}`
  updateScreenPolling()
}

// Live simulator mirror, only while a slide with a phone is showing.
let pollTimer: number | undefined
function updateScreenPolling() {
  clearTimeout(pollTimer)
  const phone = slides[current]?.querySelector<HTMLElement>('.phone')
  if (phone) {
    pollScreen(phone)
  }
}

async function pollScreen(phone: HTMLElement) {
  const img = phone.querySelector('img')!
  try {
    const response = await fetch(`/api/screen?t=${Date.now()}`)
    if (response.ok) {
      const url = URL.createObjectURL(await response.blob())
      const old = img.src
      img.src = url
      if (old.startsWith('blob:')) {
        URL.revokeObjectURL(old)
      }
    }
    phone.classList.toggle('live', response.ok)
  } catch {
    phone.classList.remove('live')
  }
  if (slides[current]?.contains(phone)) {
    pollTimer = window.setTimeout(() => pollScreen(phone), SCREEN_POLL_MS)
  }
}

async function post(path: string) {
  const response = await fetch(path, {
    method: 'POST',
    headers: { 'X-Brave-Slides': '1' },
  })
  if (!response.ok) {
    throw new Error(await response.text())
  }
}

async function onButton(button: HTMLButtonElement) {
  const { action, open } = button.dataset
  const path = open ? `/api/open/${open}` : `/api/${action}`
  button.classList.add('busy')
  try {
    await post(path)
  } catch (e) {
    console.error(e)
  } finally {
    setTimeout(() => button.classList.remove('busy'), 300)
  }
}

function followLog() {
  const logs = [...document.querySelectorAll<HTMLElement>('.log')]
  const events = new EventSource('/api/log')
  events.onmessage = (event) => {
    const line = JSON.parse(event.data) as string
    for (const log of logs) {
      // The log is laid out bottom-up, so new lines go first.
      const row = document.createElement('div')
      row.textContent = line
      log.prepend(row)
      while (log.childElementCount > 60) {
        log.lastElementChild!.remove()
      }
    }
  }
}

async function loadWatermark() {
  const response = await fetch('/lion-frame.svg')
  watermark.innerHTML = await response.text()
  watermark.querySelectorAll('path').forEach((path) => {
    path.style.setProperty('--len', String(Math.ceil(path.getTotalLength())))
  })
  deck.append(watermark, counter)
  // Drawn in on the title slide only; elsewhere it is already on the paper.
  if (current === 0) {
    watermark.classList.add('drawing')
  }
}

function onKey(event: KeyboardEvent) {
  if (event.target instanceof HTMLButtonElement && event.key === ' ') {
    return
  }
  switch (event.key) {
    case 'ArrowRight':
    case 'PageDown':
    case ' ':
      show(current + 1)
      break
    case 'ArrowLeft':
    case 'PageUp':
      show(current - 1)
      break
    case 'Home':
      show(0)
      break
    case 'End':
      show(slides.length - 1)
      break
    case 'f':
      if (document.fullscreenElement) {
        document.exitFullscreen()
      } else {
        document.documentElement.requestFullscreen()
      }
      break
    default:
      return
  }
  event.preventDefault()
}

slides.forEach(stagger)
fitToWindow()
addEventListener('resize', fitToWindow)
addEventListener('keydown', onKey)
document.addEventListener('click', (event) => {
  const button = (event.target as Element).closest('button')
  if (button) {
    onButton(button)
  }
})
loadWatermark()
followLog()
show(Number(location.hash.slice(1)) - 1 || 0)
