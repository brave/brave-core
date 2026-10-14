// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import '@testing-library/jest-dom'
import { act, render, screen } from '@testing-library/react'
import * as React from 'react'
import * as Mojom from '../../../common/mojom'
import MockContext from '../../mock_untrusted_conversation_context'
import MemoriesSentEvent from './memories_sent_event'

// The setup of the tests drops the values that go into a string. These tests
// need the values, so the string keeps them after its key.
jest.mock('$web-common/locale', () => ({
  getLocale: (key: string) => key,
  formatLocale: (key: string, replacements: Record<string, unknown> = {}) =>
    [key, ...Object.values(replacements)].join(' '),
}))

// A mojo time is in microseconds since 1601.
const WINDOWS_TO_UNIX_EPOCH_MS = 11644473600000
const mojoTime = (unixMs: number) => ({
  internalValue: BigInt(unixMs + WINDOWS_TO_UNIX_EPOCH_MS) * BigInt(1000),
})

const memory = (
  kind: Mojom.SentMemoryKind,
  text: string,
  date?: number,
): Mojom.SentMemory => ({
  kind,
  text,
  date: date === undefined ? undefined : mojoTime(date),
})

function renderEvent(
  memories: Mojom.SentMemory[],
  uiHandler: Partial<Mojom.UntrustedUIHandlerInterface> = {},
) {
  return render(
    <MockContext uiHandler={uiHandler}>
      <MemoriesSentEvent event={{ memories }} />
    </MockContext>,
  )
}

describe('MemoriesSentEvent', () => {
  test('renders nothing when no memory was sent', () => {
    const { container } = renderEvent([])
    expect(container.innerHTML).toBe('')
  })

  test('is closed until the user opens it', () => {
    const { container } = renderEvent([
      memory(Mojom.SentMemoryKind.kWritten, 'I like tea'),
    ])
    const collapse = container.querySelector('leo-collapse') as any
    expect(collapse).toBeInTheDocument()
    expect(collapse.isOpen).toBe(false)

    // Nala's Collapse tells the toggle of the user to its parent.
    act(() => collapse.onToggle({ open: true }))
    expect((container.querySelector('leo-collapse') as any).isOpen).toBe(true)
    act(() => collapse.onToggle({ open: false }))
    expect((container.querySelector('leo-collapse') as any).isOpen).toBe(false)
  })

  test('shows the number of memories in the title', () => {
    renderEvent([
      memory(Mojom.SentMemoryKind.kName, 'Jocelyn'),
      memory(Mojom.SentMemoryKind.kWritten, 'I like tea'),
      memory(Mojom.SentMemoryKind.kLearned, 'Lives in Italy'),
    ])
    expect(
      screen.getByText(`${S.CHAT_UI_MEMORIES_SENT_TITLE} 3`),
    ).toBeInTheDocument()
  })

  test('puts each memory in the group of its kind', () => {
    renderEvent([
      memory(Mojom.SentMemoryKind.kName, 'Jocelyn'),
      memory(Mojom.SentMemoryKind.kOtherInstructions, 'Be brief'),
      memory(Mojom.SentMemoryKind.kWritten, 'I like tea'),
      memory(Mojom.SentMemoryKind.kLearned, 'Lives in Italy'),
    ])

    const headings = screen.getAllByRole('heading', { level: 4 })
    expect(headings.map((heading) => heading.textContent)).toEqual([
      S.CHAT_UI_MEMORIES_SENT_CUSTOMIZATION_HEADING,
      S.CHAT_UI_MEMORIES_SENT_WRITTEN_HEADING,
      S.CHAT_UI_MEMORIES_SENT_LEARNED_HEADING,
    ])
    const lists = screen.getAllByRole('list')
    expect(lists).toHaveLength(3)
    // The customization shows its value after the label of its field.
    expect(lists[0]).toHaveTextContent(
      `${S.CHAT_UI_MEMORIES_SENT_NAME_LABEL} Jocelyn`,
    )
    expect(lists[0]).toHaveTextContent(
      `${S.CHAT_UI_MEMORIES_SENT_OTHER_LABEL} Be brief`,
    )
    expect(lists[1]).toHaveTextContent('I like tea')
    expect(lists[2]).toHaveTextContent('Lives in Italy')
  })

  test('leaves out a group without memories', () => {
    renderEvent([memory(Mojom.SentMemoryKind.kWritten, 'I like tea')])
    expect(screen.getAllByRole('heading', { level: 4 })).toHaveLength(1)
    expect(screen.getAllByRole('list')).toHaveLength(1)
  })

  test('shows the date of a learned memory in the language of the user', () => {
    const date = Date.UTC(2026, 9, 13, 12)
    renderEvent([memory(Mojom.SentMemoryKind.kLearned, 'Lives in Italy', date)])
    const expected = new Date(date).toLocaleDateString(undefined, {
      dateStyle: 'medium',
    })
    expect(
      screen.getByText(`${S.CHAT_UI_MEMORIES_SENT_LAST_MENTIONED} ${expected}`),
    ).toBeInTheDocument()
  })

  test('has no date for a memory that the user wrote', () => {
    renderEvent([memory(Mojom.SentMemoryKind.kWritten, 'I like tea')])
    expect(screen.getByRole('listitem').querySelector('span')).toBeNull()
  })

  test('opens the settings from the manage button', () => {
    const openSettings = jest.fn()
    renderEvent([memory(Mojom.SentMemoryKind.kWritten, 'I like tea')], {
      openAIChatCustomizationSettings: openSettings,
    })
    // Nala's Button tells its parent about the click through `onClick`.
    act(() =>
      (screen.getByTestId('memories-sent-manage-button') as any).onClick(),
    )
    expect(openSettings).toHaveBeenCalledTimes(1)
  })
})
