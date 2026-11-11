// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { act, fireEvent, render, screen } from '@testing-library/react'
import { renderShareCard } from '../share_card'
import { App } from './app'

jest.mock('../share_card', () => ({ renderShareCard: jest.fn() }))
const mockRenderShareCard = renderShareCard as jest.MockedFunction<
  typeof renderShareCard
>

const DAY_MS = 24 * 60 * 60 * 1000
const GB = 1024 * 1024 * 1024

function renderApp(
  adsBlocked: number,
  bandwidthSavedBytes = 5 * GB,
  firstRunTime = Date.now() - 731 * DAY_MS,
) {
  return render(
    <App stats={{ adsBlocked, bandwidthSavedBytes, firstRunTime }} />,
  )
}

function heading(level: number) {
  return screen.getByRole('heading', { level })
}

function pressKey(key: string) {
  fireEvent.keyDown(window, { key })
}

// Leo buttons ignore fireEvent's synthetic clicks, so use a native one.
function click(element: HTMLElement) {
  act(() => element.click())
}

function clickText(text: string) {
  click(screen.getByText(text))
}

function progress() {
  return screen.getByRole('progressbar')
}

function frames() {
  return [...document.querySelectorAll<HTMLElement>('.slide-frame')]
}

function currentFrame() {
  return document.querySelector<HTMLElement>('.slide-frame:not([inert])')!
}

// jsdom doesn't run CSS animations, so finish the slides that are leaving.
function finishExits() {
  for (const frame of document.querySelectorAll('.slide-frame[inert]')) {
    fireEvent.transitionEnd(frame)
  }
}

function runningStep() {
  return progress().querySelector<HTMLElement>('.fill.running')
}

function mockMatchMedia(reducedMotion: boolean) {
  jest.spyOn(window, 'matchMedia').mockImplementation(
    (query) =>
      ({
        matches: reducedMotion && query === '(prefers-reduced-motion: reduce)',
        media: query,
        addEventListener: () => {},
        removeEventListener: () => {},
      }) as unknown as MediaQueryList,
  )
}

describe('brave recap app', () => {
  beforeEach(() => {
    mockMatchMedia(false)
    mockRenderShareCard.mockResolvedValue(new Blob())
  })
  afterEach(() => jest.restoreAllMocks())

  it('shows the empty state below 100 blocked items', () => {
    renderApp(99)
    expect(heading(1)).toHaveTextContent('BRAVE_RECAP_EMPTY_HEADING')
    expect(screen.queryByRole('progressbar')).not.toBeInTheDocument()
  })

  it('shows the intro once enough has been blocked', () => {
    renderApp(100)
    expect(heading(1)).toHaveTextContent('BRAVE_RECAP_INTRO_HEADING')
    expect(progress()).toHaveAttribute('aria-valuenow', '1')
  })

  it('shows the install month and days on the intro when known', () => {
    renderApp(100)
    expect(screen.getByText('BRAVE_RECAP_INTRO_SINCE')).toBeInTheDocument()
    expect(screen.getByText('BRAVE_RECAP_INTRO_DAYS')).toBeInTheDocument()
  })

  it('falls back to generic intro text without an install date', () => {
    renderApp(100, 0, 0)
    expect(screen.getByText('BRAVE_RECAP_INTRO_NO_DATE')).toBeInTheDocument()
    expect(screen.queryByText('BRAVE_RECAP_INTRO_DAYS')).not.toBeInTheDocument()
  })

  it('explains where the data stays from the help button', () => {
    renderApp(100)
    expect(screen.getByTestId('recap-help')).toBeInTheDocument()
    expect(screen.getByText('BRAVE_RECAP_PRIVACY_NOTE')).toBeInTheDocument()
  })

  it('moves between slides with the buttons and keyboard', () => {
    renderApp(100)
    clickText('BRAVE_RECAP_GET_STARTED')
    expect(heading(2)).toHaveTextContent('BRAVE_RECAP_BLOCKED_HEADING')

    clickText('BRAVE_RECAP_NEXT')
    expect(heading(2)).toHaveTextContent('BRAVE_RECAP_TIME_CAPTION')

    pressKey('ArrowRight')
    expect(heading(2)).toHaveTextContent('BRAVE_RECAP_DATA_CAPTION')

    pressKey('ArrowLeft')
    click(screen.getByTestId('recap-back'))
    expect(heading(2)).toHaveTextContent('BRAVE_RECAP_BLOCKED_HEADING')
  })

  it('advances when the slide is tapped', () => {
    renderApp(100)
    fireEvent.click(heading(1))
    expect(heading(2)).toHaveTextContent('BRAVE_RECAP_BLOCKED_HEADING')
  })

  describe('tap zones', () => {
    // A 300px wide slide whose start third is the first 100px.
    function tapAt(clientX: number) {
      const slide = currentFrame().querySelector<HTMLElement>('.slide')!
      jest
        .spyOn(slide, 'getBoundingClientRect')
        .mockReturnValue({ left: 0, right: 300, width: 300 } as DOMRect)
      fireEvent.click(slide, { clientX })
    }

    it('goes back from the start third and advances from the rest', () => {
      renderApp(100)
      tapAt(150)
      expect(progress()).toHaveAttribute('aria-valuenow', '2')
      tapAt(250)
      expect(progress()).toHaveAttribute('aria-valuenow', '3')
      tapAt(50)
      expect(progress()).toHaveAttribute('aria-valuenow', '2')
    })

    it('goes back from the right in right-to-left layouts', () => {
      renderApp(100)
      tapAt(150)
      jest
        .spyOn(window, 'getComputedStyle')
        .mockReturnValue({ direction: 'rtl' } as CSSStyleDeclaration)
      tapAt(250)
      expect(progress()).toHaveAttribute('aria-valuenow', '1')
    })
  })

  it('shows the blocked count and the comparison for it', () => {
    mockMatchMedia(true)
    renderApp(274_184)
    pressKey('ArrowRight')
    expect(screen.getAllByText('274,184').length).toBeGreaterThan(0)
    expect(
      screen.getByText('BRAVE_RECAP_BLOCKED_COMPARISON_270000'),
    ).toBeInTheDocument()
    expect(screen.getByText('BRAVE_RECAP_BLOCKED_TEXT')).toBeInTheDocument()
  })

  it('reads out the final count while it is counting', () => {
    renderApp(274_184)
    pressKey('ArrowRight')
    const value = currentFrame().querySelector('.big-value')!
    expect(value.querySelector('.sizer')).toHaveTextContent('274,184')
    expect(value.querySelector('.sizer')).not.toHaveAttribute('aria-hidden')
    expect(value.lastElementChild).toHaveAttribute('aria-hidden', 'true')
  })

  it('skips the comparison below the first milestone', () => {
    // 100 blocked saves 5 seconds, below the first time milestone.
    renderApp(100)
    pressKey('ArrowRight')
    pressKey('ArrowRight')
    expect(screen.getByText('BRAVE_RECAP_TIME_TEXT')).toBeInTheDocument()
    expect(
      currentFrame().querySelector('.panel.comparison'),
    ).not.toBeInTheDocument()
  })

  it('compares data saved to the highest milestone reached', () => {
    renderApp(100, 8.83 * GB)
    pressKey('ArrowRight')
    pressKey('ArrowRight')
    pressKey('ArrowRight')
    expect(
      screen.getByText('BRAVE_RECAP_DATA_COMPARISON_8GB'),
    ).toBeInTheDocument()
  })
})

describe('brave recap slide transitions', () => {
  beforeEach(() => {
    mockMatchMedia(false)
    mockRenderShareCard.mockReturnValue(new Promise(() => {}))
  })
  afterEach(() => jest.restoreAllMocks())

  it('does not animate the first slide in', () => {
    renderApp(100)
    expect(frames()).toHaveLength(1)
    expect(currentFrame()).not.toHaveAttribute('data-motion')
  })

  it('pushes the next slide in while the last one leaves', () => {
    renderApp(100)
    pressKey('ArrowRight')
    const [leaving, entering] = frames()
    expect(leaving).toHaveAttribute('data-motion', 'exit')
    expect(leaving).toHaveAttribute('data-direction', 'forward')
    expect(leaving).toHaveAttribute('aria-hidden', 'true')
    expect(entering).toHaveAttribute('data-motion', 'enter')
    expect(entering).toHaveAttribute('data-direction', 'forward')
  })

  it('pushes the other way when going back', () => {
    renderApp(100)
    pressKey('ArrowRight')
    finishExits()
    pressKey('ArrowLeft')
    const [entering, leaving] = frames()
    expect(entering).toHaveAttribute('data-motion', 'enter')
    expect(entering).toHaveAttribute('data-direction', 'back')
    expect(leaving).toHaveAttribute('data-motion', 'exit')
    expect(leaving).toHaveAttribute('data-direction', 'back')
  })

  it('removes the leaving slide once it is out', () => {
    renderApp(100)
    pressKey('ArrowRight')
    finishExits()
    expect(frames()).toHaveLength(1)
    expect(heading(2)).toHaveTextContent('BRAVE_RECAP_BLOCKED_HEADING')
  })

  it('ignores transitions inside the leaving slide', () => {
    renderApp(100)
    pressKey('ArrowRight')
    fireEvent.transitionEnd(frames()[0].querySelector('.message')!)
    expect(frames()).toHaveLength(2)
  })

  it('can reverse a slide that is still leaving', () => {
    renderApp(100)
    pressKey('ArrowRight')
    pressKey('ArrowLeft')
    const [intro, blocked] = frames()
    expect(intro).not.toHaveAttribute('inert')
    expect(intro).toHaveAttribute('data-motion', 'enter')
    expect(blocked).toHaveAttribute('data-motion', 'exit')
    expect(blocked).toHaveAttribute('data-direction', 'back')
  })

  it('keeps one set of buttons while slides move', () => {
    renderApp(100)
    pressKey('ArrowRight')
    pressKey('ArrowRight')
    expect(screen.getAllByTestId('recap-back')).toHaveLength(1)
    expect(screen.getAllByText('BRAVE_RECAP_NEXT')).toHaveLength(1)
  })

  it('changes slides in place with reduced motion', () => {
    mockMatchMedia(true)
    renderApp(100)
    pressKey('ArrowRight')
    expect(frames()).toHaveLength(1)
  })
})

describe('brave recap auto-advance', () => {
  beforeEach(() => {
    mockMatchMedia(false)
    mockRenderShareCard.mockReturnValue(new Promise(() => {}))
  })
  afterEach(() => jest.restoreAllMocks())

  // The first timed slide.
  function renderOnBlocked() {
    renderApp(100)
    pressKey('ArrowRight')
  }

  function backgrounds() {
    return document.querySelector('.backgrounds')!
  }

  it('waits on the intro until the person starts', () => {
    renderApp(100)
    expect(runningStep()).toBeNull()
    expect(progress().querySelector('.fill')).toHaveClass('done')
    clickText('BRAVE_RECAP_GET_STARTED')
    expect(runningStep()).not.toBeNull()
  })

  it('advances when the current step finishes', () => {
    renderOnBlocked()
    fireEvent.animationEnd(runningStep()!)
    expect(heading(2)).toHaveTextContent('BRAVE_RECAP_TIME_CAPTION')
    expect(progress()).toHaveAttribute('aria-valuenow', '3')
  })

  it('fills earlier steps and leaves later ones empty', () => {
    renderApp(100)
    pressKey('ArrowRight')
    pressKey('ArrowRight')
    const fills = progress().querySelectorAll('.fill')
    expect([...fills].map((fill) => fill.classList[1])).toEqual([
      'done',
      'done',
      'running',
      'todo',
      'todo',
    ])
  })

  it('pauses the step and the photo while the slide is held', () => {
    renderOnBlocked()
    const card = screen.getByRole('main')
    fireEvent.pointerDown(card)
    expect(runningStep()).toHaveAttribute('data-paused')
    expect(backgrounds()).toHaveAttribute('data-paused')
    fireEvent.pointerUp(card)
    expect(runningStep()).not.toHaveAttribute('data-paused')
    expect(backgrounds()).not.toHaveAttribute('data-paused')
  })

  it('hides the controls only while the slide content is held', () => {
    renderOnBlocked()
    const card = screen.getByRole('main')
    fireEvent.pointerDown(heading(2))
    expect(card).toHaveAttribute('data-holding')
    fireEvent.pointerUp(card)
    expect(card).not.toHaveAttribute('data-holding')
    fireEvent.pointerDown(screen.getByTestId('recap-back'))
    expect(card).not.toHaveAttribute('data-holding')
  })

  it('does not advance when a long press is released', () => {
    const now = jest.spyOn(performance, 'now').mockReturnValue(0)
    renderApp(100)
    fireEvent.pointerDown(screen.getByRole('main'))
    now.mockReturnValue(1000)
    fireEvent.pointerUp(screen.getByRole('main'))
    fireEvent.click(heading(1))
    expect(heading(1)).toHaveTextContent('BRAVE_RECAP_INTRO_HEADING')
  })

  it('holds the photo still while the help tooltip is open', () => {
    renderApp(100)
    // Leo forwards the React handler to the tooltip's callback prop.
    const tooltip = screen.getByTestId('recap-help').parentElement as any
    act(() => tooltip.onVisibilityChange({ visible: true }))
    expect(backgrounds()).toHaveAttribute('data-paused')
  })

  it('pauses while the page is hidden', () => {
    renderOnBlocked()
    jest.spyOn(document, 'hidden', 'get').mockReturnValue(true)
    act(() => {
      document.dispatchEvent(new Event('visibilitychange'))
    })
    expect(runningStep()).toHaveAttribute('data-paused')
  })

  it('pauses while a control has keyboard focus', () => {
    renderOnBlocked()
    pressKey('Tab')
    fireEvent.focusIn(screen.getByTestId('recap-back'))
    expect(runningStep()).toHaveAttribute('data-paused')
  })

  it('does not auto-advance with reduced motion', () => {
    mockMatchMedia(true)
    renderOnBlocked()
    expect(runningStep()).toBeNull()
    expect(progress().querySelectorAll('.fill')[1]).toHaveClass('done')
  })

  it('stops on the share slide', () => {
    renderApp(100)
    for (let i = 0; i < 4; i++) {
      pressKey('ArrowRight')
    }
    expect(runningStep()).toBeNull()
    pressKey('ArrowRight')
    expect(progress()).toHaveAttribute('aria-valuenow', '5')
  })
})

describe('brave recap share slide', () => {
  const SHARE_TEXT = 'BRAVE_RECAP_SHARE_POST_TEXT https://brave.com/download/'

  let card: Blob
  let createObjectURL: jest.Mock
  let revokeObjectURL: jest.Mock

  beforeEach(() => {
    mockMatchMedia(false)
    card = new Blob([], { type: 'image/png' })
    mockRenderShareCard.mockResolvedValue(card)
    createObjectURL = jest.fn(() => 'blob:card')
    revokeObjectURL = jest.fn()
    Object.assign(URL, { createObjectURL, revokeObjectURL })
  })
  afterEach(() => jest.restoreAllMocks())

  function renderShareSlide() {
    const result = renderApp(1_284_213)
    for (let i = 0; i < 4; i++) {
      pressKey('ArrowRight')
    }
    return result
  }

  it('previews the card once it has rendered', async () => {
    renderShareSlide()
    expect(await screen.findByRole('img')).toHaveAttribute('src', 'blob:card')
    expect(mockRenderShareCard).toHaveBeenCalledWith(
      expect.objectContaining({
        title: 'BRAVE_RECAP_CARD_TITLE',
        url: 'brave://recap',
      }),
      { logo: '/brave-icon.svg', pattern: '/images/card_pattern.png' },
    )
  })

  it('renders the card as soon as the recap opens', async () => {
    renderApp(1_284_213)
    expect(mockRenderShareCard).toHaveBeenCalledTimes(1)
    await act(async () => {})
    expect(createObjectURL).toHaveBeenCalled()
    for (let i = 0; i < 4; i++) {
      pressKey('ArrowRight')
    }
    // Already there on arrival, not rendered again.
    expect(screen.getByRole('img')).toHaveAttribute('src', 'blob:card')
    expect(mockRenderShareCard).toHaveBeenCalledTimes(1)
  })

  it('keeps the card when leaving the slide and revokes it on close', async () => {
    const { unmount } = renderShareSlide()
    await screen.findByRole('img')
    pressKey('ArrowLeft')
    finishExits()
    expect(revokeObjectURL).not.toHaveBeenCalled()
    unmount()
    expect(revokeObjectURL).toHaveBeenCalledWith('blob:card')
  })

  it('does not create a card URL after leaving before it renders', async () => {
    let resolve!: (blob: Blob) => void
    mockRenderShareCard.mockReturnValue(new Promise((r) => (resolve = r)))
    const { unmount } = renderShareSlide()
    unmount()
    await act(async () => resolve(new Blob()))
    expect(createObjectURL).not.toHaveBeenCalled()
  })

  it('logs when the card fails to render', async () => {
    const error = jest.spyOn(console, 'error').mockImplementation(() => {})
    mockRenderShareCard.mockRejectedValue(new Error('toBlob failed'))
    renderShareSlide()
    await act(async () => {})
    expect(error).toHaveBeenCalledWith(
      '[brave-recap] failed to render share card:',
      expect.any(Error),
    )
    expect(screen.queryByRole('img')).not.toBeInTheDocument()
  })

  it('downloads the card', async () => {
    const linkClick = jest
      .spyOn(HTMLAnchorElement.prototype, 'click')
      .mockImplementation(function (this: HTMLAnchorElement) {
        expect(this.href).toBe('blob:card')
        expect(this.download).toBe('brave-recap.png')
      })
    renderShareSlide()
    await screen.findByRole('img')
    clickText('BRAVE_RECAP_DOWNLOAD')
    expect(linkClick).toHaveBeenCalled()
  })

  describe('posting', () => {
    let write: jest.Mock
    let open: jest.SpyInstance
    let ClipboardItem: jest.Mock

    beforeEach(() => {
      write = jest.fn().mockResolvedValue(undefined)
      Object.assign(navigator, { clipboard: { write } })
      ClipboardItem = jest.fn()
      Object.assign(window, { ClipboardItem })
      open = jest.spyOn(window, 'open').mockImplementation(() => null)
    })

    async function post(button: string) {
      renderShareSlide()
      await screen.findByRole('img')
      await act(async () => screen.getByText(button).click())
    }

    it('opens an X post with the share text', async () => {
      await post('BRAVE_RECAP_POST_ON_X')
      expect(open).toHaveBeenCalledWith(
        `https://x.com/intent/post?text=${encodeURIComponent(SHARE_TEXT)}`,
        '_blank',
        'noopener',
      )
    })

    it('opens a Reddit text post with the same text as X', async () => {
      await post('BRAVE_RECAP_POST_ON_REDDIT')
      const url = new URL(open.mock.calls[0][0] as string)
      expect(url.origin + url.pathname).toBe('https://www.reddit.com/submit')
      expect(url.searchParams.get('selftext')).toBe('true')
      expect(url.searchParams.get('title')).toBe('BRAVE_RECAP_SHARE_POST_TITLE')
      expect(url.searchParams.get('text')).toBe(SHARE_TEXT)
    })

    it.each(['BRAVE_RECAP_POST_ON_X', 'BRAVE_RECAP_POST_ON_REDDIT'])(
      'copies the image to paste in before %s opens',
      async (button) => {
        await post(button)
        expect(ClipboardItem).toHaveBeenCalledWith({ 'image/png': card })
        expect(write).toHaveBeenCalled()
        expect(write.mock.invocationCallOrder[0]).toBeLessThan(
          open.mock.invocationCallOrder[0],
        )
        expect(screen.getByRole('status')).toHaveTextContent(
          'BRAVE_RECAP_IMAGE_COPIED',
        )
      },
    )

    it('still opens the post if the image cannot be copied', async () => {
      jest.spyOn(console, 'error').mockImplementation(() => {})
      write.mockRejectedValue(new Error('Document is not focused'))
      await post('BRAVE_RECAP_POST_ON_X')
      expect(open).toHaveBeenCalled()
      expect(screen.queryByRole('status')).not.toBeInTheDocument()
    })
  })
})
