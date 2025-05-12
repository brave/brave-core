// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, fireEvent, waitFor, act } from '@testing-library/react'
import { ManagePage } from '../email_aliases'
import { localeRegex, clickLeoButton } from './test_utils'
import {
  EmailAliasesServiceInterface,
  EmailAliasesServiceObserverInterface
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

jest.mock('$web-common/locale', () => ({
  getLocale: (key: string) => {
    return key
  },
  formatMessage: (key: string, params: Record<string, string>) => {
    return key
  },
  formatLocale: (key: string, params: Record<string, string>) => {
    return key
  }
}))

// Mock the email aliases service
class MockEmailAliasesService extends EmailAliasesServiceInterface {
  private observerInit?:
    (observer: EmailAliasesServiceObserverInterface) => void
  constructor(observerInit?:
    (observer: EmailAliasesServiceObserverInterface) => void) {
    super()
    this.observerInit = observerInit
  }

  addObserver(observer: EmailAliasesServiceObserverInterface) {
    this.observerInit?.(observer)
  }
  removeObserver(observer: EmailAliasesServiceObserverInterface) {}

  generateAlias = jest.fn()
  updateAlias = jest.fn()
  deleteAlias = jest.fn()
  requestPrimaryEmailVerification = jest.fn()
  cancelPrimaryEmailVerification = jest.fn()
  logout = jest.fn()
  showSettingsPage = jest.fn()
}

const createBindObserver =
  (emailAliasesService: MockEmailAliasesService) =>
  (observer: EmailAliasesServiceObserverInterface) => {
    emailAliasesService.addObserver(observer)
    return () => {
      emailAliasesService.removeObserver(observer)
    }
  }

describe('ManagePage', () => {
  beforeEach(() => {
    jest.clearAllMocks()
  })

  it('shows loading state initially', async () => {
    const mockEmailAliasesService = new MockEmailAliasesService()
    await act(async () => {
      render(<ManagePage
               emailAliasesService={mockEmailAliasesService}
               bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })
    expect(document.querySelector('leo-progressring'))
      .toBeInTheDocument()
    expect(screen.getByText(localeRegex(
      'emailAliasesConnectingToBraveAccount'))).toBeInTheDocument()
  })

  it('shows sign up form when no email is available', async () => {
    const mockEmailAliasesService =
      new MockEmailAliasesService((observer) => {
      observer.onLoggedOut()
    })
    await act(async () => {
      render(<ManagePage
               emailAliasesService={mockEmailAliasesService}
               bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })

    await waitFor(() => {
      expect(screen.getByText(localeRegex('emailAliasesSignInOrCreateAccount')))
        .toBeInTheDocument()
      expect(screen.getByPlaceholderText(localeRegex(
        'emailAliasesEmailAddressPlaceholder'))).toBeInTheDocument()
    })
  })

  it('shows main view when email is available', async () => {
    const mockEmail = 'test@brave.com'
    const mockAliases = [
      {
        email: 'alias1@brave.com',
        note: 'Test Alias 1',
        domains: undefined,
      }
    ]
    const mockEmailAliasesService =
      new MockEmailAliasesService((observer) => {
      observer.onLoggedIn(mockEmail)
      observer.onAliasesUpdated(mockAliases)
    })

    await act(async () => {
      render(<ManagePage emailAliasesService={mockEmailAliasesService}
            bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })

    await waitFor(() => {
      expect(screen.getByText(mockEmail)).toBeInTheDocument()
      expect(screen.getByText(/alias1@brave\.com/)).toBeInTheDocument()
      expect(screen.getByText(/Test Alias 1/)).toBeInTheDocument()
    })
  })

  it('shows verification pending view', async () => {
    const mockEmail = 'test@brave.com'
    const mockEmailAliasesService =
      new MockEmailAliasesService((observer) => {
      observer.onVerificationPending(mockEmail)
    })

    await act(async () => {
      render(<ManagePage emailAliasesService={mockEmailAliasesService}
            bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })

    await waitFor(() => {
      expect(screen.getByText(localeRegex('emailAliasesLoginEmailOnTheWay')))
        .toBeInTheDocument()
      expect(screen.getByText(localeRegex('emailAliasesClickOnSecureLogin')))
        .toBeInTheDocument()
      expect(screen.getByText(localeRegex('emailAliasesDontSeeEmail')))
        .toBeInTheDocument()
    })
  })

  it('triggers logout when sign out button is clicked', async () => {
    const mockEmail = 'test@brave.com'
    const mockEmailAliasesService =
      new MockEmailAliasesService((observer) => {
      observer.onLoggedIn(mockEmail)
    })

    await act(async () => {
      render(<ManagePage emailAliasesService={mockEmailAliasesService}
            bindObserver={createBindObserver(mockEmailAliasesService)}
      />)
    })

    await waitFor(() => {
      expect(screen.getByText(localeRegex('emailAliasesSignOut')))
        .toBeInTheDocument()
    })

    fireEvent.click(screen.getByText(localeRegex('emailAliasesSignOut')))

    expect(mockEmailAliasesService.logout).toHaveBeenCalled()
  })

})
