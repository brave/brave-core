// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { Meta } from '@storybook/react'
import * as Mojom from '../../../common/mojom'
import MockContext from '../../mock_untrusted_conversation_context'
import MemoriesSentEvent from './memories_sent_event'

// A mojo time is in microseconds since 1601.
const WINDOWS_TO_UNIX_EPOCH_MS = 11644473600000
const mojoTime = (date: Date) => ({
  internalValue:
    BigInt(date.getTime() + WINDOWS_TO_UNIX_EPOCH_MS) * BigInt(1000),
})

const memory = (
  kind: Mojom.SentMemoryKind,
  text: string,
  date?: Date,
): Mojom.SentMemory => ({
  kind,
  text,
  date: date ? mojoTime(date) : undefined,
})

function Story(props: { memories: Mojom.SentMemory[] }) {
  return (
    <MockContext>
      <MemoriesSentEvent event={{ memories: props.memories }} />
    </MockContext>
  )
}

export const AllKinds = () => (
  <Story
    memories={[
      memory(Mojom.SentMemoryKind.kName, 'Jocelyn'),
      memory(Mojom.SentMemoryKind.kJob, 'Product designer'),
      memory(Mojom.SentMemoryKind.kTone, 'Short and direct'),
      memory(Mojom.SentMemoryKind.kOtherInstructions, 'Use metric units'),
      memory(Mojom.SentMemoryKind.kWritten, 'I like running'),
      memory(Mojom.SentMemoryKind.kWritten, 'I live in the Bay Area'),
      memory(
        Mojom.SentMemoryKind.kLearned,
        'Is vegetarian and prefers no meat recipes.',
        new Date(2026, 8, 16),
      ),
      memory(
        Mojom.SentMemoryKind.kLearned,
        'Moved to Italy.',
        new Date(2026, 9, 13),
      ),
    ]}
  />
)

export const OnlyMemoriesYouAdded = () => (
  <Story
    memories={[
      memory(Mojom.SentMemoryKind.kWritten, 'I like running'),
      memory(Mojom.SentMemoryKind.kWritten, 'Likes Taiwan'),
    ]}
  />
)

export const LongText = () => (
  <Story
    memories={[
      memory(
        Mojom.SentMemoryKind.kOtherInstructions,
        'Always answer in British English, explain each step of a recipe in '
          + 'detail, give the amounts in grams and millilitres, and never '
          + 'suggest a restaurant that needs a booking weeks ahead.',
      ),
      memory(
        Mojom.SentMemoryKind.kLearned,
        'Averyveryveryverylongwordwithoutanyspacesinitthatmustwrapinsteadof'
          + 'growingthecardbeyonditsedge',
        new Date(2026, 9, 13),
      ),
    ]}
  />
)

export default {
  title: 'AI Chat/MemoriesSentEvent',
  component: MemoriesSentEvent,
} as Meta<typeof MemoriesSentEvent>
