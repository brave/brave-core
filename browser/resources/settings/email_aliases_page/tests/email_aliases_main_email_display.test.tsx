// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { render, screen, act } from '@testing-library/react'
import { MainEmailDisplay } from '../content/email_aliases_main_email_display'
import { EmailAliasesServiceInterface } from 'gen/brave/components/email_aliases/email_aliases.mojom.m'
import { clickLeoButton } from './test_utils'
import * as React from 'react'
import { getLocale } from '$web-common/locale'

jest.mock('$web-common/locale', () => ({
  getLocale: (key: string) => {
    return key
  },
}))

const mockEmailAliasesService: EmailAliasesServiceInterface = {
  requestAuthentication: jest.fn(),
  cancelAuthenticationOrLogout: jest.fn(),
  generateAlias: jest.fn(),
  updateAlias: jest.fn(),
  deleteAlias: jest.fn(),
  addObserver: jest.fn(),
  removeObserver: jest.fn(),
}

describe('MainEmailDisplay', () => {
  it('renders the main email display', async () => {
    const mockEmail = 'test@example.com'

    render(<MainEmailDisplay
      email={mockEmail}
      emailAliasesService={mockEmailAliasesService} />)

    expect(screen.getByText('test@example.com')).toBeInTheDocument()

    const signOutButton = screen.getByTitle(getLocale('emailAliasesSignOutTitle'))
    await act(async () => {
      clickLeoButton(signOutButton)
    })
    expect(mockEmailAliasesService.cancelAuthenticationOrLogout).toHaveBeenCalled()
  })
})
