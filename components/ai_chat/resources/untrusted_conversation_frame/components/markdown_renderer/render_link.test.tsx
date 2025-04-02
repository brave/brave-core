// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import '@testing-library/jest-dom'
import { render, screen } from '@testing-library/react'
import { RenderLink } from '.'

test('Test RenderLink component with allowed links.', async () => {
  render(
    <RenderLink
      a={{ href: 'https://example.com', children: 'Test Link' }}
      allowedLinks={['https://example.com']}
    />
  )
  expect(screen.getByText('Test Link')).toBeInTheDocument()
  expect(screen.getByText('Test Link').tagName).toBe('BUTTON')
})

test('Test RenderLink component with disallowed links.', async () => {
  render(
    <RenderLink
      a={{ href: 'https://example.com', children: 'Test Link' }}
      allowedLinks={['https://brave.com']}
    />
  )
  expect(screen.getByText('Test Link')).toBeInTheDocument()
  expect(screen.getByText('Test Link').tagName).toBe('SPAN')
})
