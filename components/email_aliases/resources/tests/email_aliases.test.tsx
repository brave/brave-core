// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor, act } from '@testing-library/react'
import { ManagePageConnected } from '../email_aliases'
import {
  Alias,
  AuthenticationStatus,
  EmailAliasesServiceInterface,
  EmailAliasesServiceObserverInterface,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'

// Mock the email aliases service
class MockEmailAliasesService extends EmailAliasesServiceInterface {
  private observer?: EmailAliasesServiceObserverInterface

  addObserver(observer: EmailAliasesServiceObserverInterface) {
    expect(this.observer).toBeUndefined()
    this.observer = observer
  }

  notifyObserverAliasesUpdated(aliases: Alias[]) {
    expect(this.observer).toBeDefined()
    this.observer?.onAliasesUpdated(aliases)
  }

  notifyObserverAuthStateChanged(
    status: AuthenticationStatus,
    email: string,
    errorMessage?: string,
  ) {
    expect(this.observer).toBeDefined()
    this.observer?.onAuthStateChanged({ status, email, errorMessage })
  }

  generateAlias = jest.fn()
  updateAlias = jest.fn()
  deleteAlias = jest.fn()
  requestAuthentication = jest.fn()
  cancelAuthentication = jest.fn()
  logout = jest.fn()
}

const createBindObserver =
  (emailAliasesService: MockEmailAliasesService) =>
  (observer: EmailAliasesServiceObserverInterface) => {
    emailAliasesService.addObserver(observer)
    return () => {}
  }

const mockEmail = 'test@brave.com'

// Test setup helpers
const setupTest = async () => {
  const mockEmailAliasesService = new MockEmailAliasesService()

  render(
    <ManagePageConnected
      emailAliasesService={mockEmailAliasesService}
      bindObserver={createBindObserver(mockEmailAliasesService)}
    />,
  )

  return mockEmailAliasesService
}

const mockAliases = [
  {
    email: 'alias1@brave.com',
    note: 'Test Alias 1',
    domains: undefined,
  },
  {
    email: 'alias2@brave.com',
    note: 'Test Alias 2',
    domains: undefined,
  },
]

const authenticate = async (
  service: MockEmailAliasesService,
  status: AuthenticationStatus,
  email: string = mockEmail,
  errorMessage?: string,
) => {
  await act(() => {
    service.notifyObserverAuthStateChanged(status, email, errorMessage)
  })
}

const updateAliases = async (
  service: MockEmailAliasesService,
  aliases = mockAliases,
) => {
  await act(() => {
    service.notifyObserverAliasesUpdated(aliases)
  })
}

const expectAliasesNotVisible = async () => {
  await waitFor(() => {
    expect(screen.queryByText('alias1@brave.com')).not.toBeInTheDocument()
    expect(screen.queryByText('alias2@brave.com')).not.toBeInTheDocument()
  })
}

describe('ManagePageConnected', () => {
  beforeEach(() => {
    jest.clearAllMocks()
  })

  it('shows loading state initially', async () => {
    await setupTest()
    await waitFor(() => {
      expect(document.querySelector('leo-progressring')).toBeInTheDocument()
      expect(
        screen.getByText('emailAliasesConnectingToBraveAccount'),
      ).toBeInTheDocument()
    })
  })

  it('shows sign up form when no email is available', async () => {
    const service = await setupTest()
    await authenticate(service, AuthenticationStatus.kUnauthenticated)

    await waitFor(() => {
      expect(
        screen.getByText('emailAliasesSignInOrCreateAccount'),
      ).toBeInTheDocument()
      expect(
        screen.getByPlaceholderText('emailAliasesEmailAddressPlaceholder'),
      ).toBeInTheDocument()
    })
  })

  it('shows main view when email is available', async () => {
    const service = await setupTest()
    await authenticate(service, AuthenticationStatus.kAuthenticated)

    await waitFor(() => {
      expect(screen.getByText(mockEmail)).toBeInTheDocument()
      expect(screen.getByText('emailAliasesBraveAccount')).toBeInTheDocument()
      expect(screen.getByText('emailAliasesSignOut')).toBeInTheDocument()
    })
  })

  it('shows verification pending view', async () => {
    const service = await setupTest()
    await authenticate(service, AuthenticationStatus.kAuthenticating)

    await waitFor(() => {
      expect(
        screen.getByText('emailAliasesLoginEmailOnTheWay'),
      ).toBeInTheDocument()
      expect(
        screen.getByText('emailAliasesClickOnSecureLogin'),
      ).toBeInTheDocument()
      expect(screen.getByText('emailAliasesDontSeeEmail')).toBeInTheDocument()
    })
  })

  it('can add new aliases via observer', async () => {
    const service = await setupTest()
    await authenticate(service, AuthenticationStatus.kAuthenticated)

    // Add an alias
    await updateAliases(service, [
      {
        email: 'alias1@brave.com',
        note: 'Test Alias 1',
        domains: undefined,
      },
    ])

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).not.toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).not.toBeInTheDocument()
    })

    // Add more aliases
    await updateAliases(service, [
      {
        email: 'alias1@brave.com',
        note: 'Test Alias 1',
        domains: undefined,
      },
      {
        email: 'alias2@brave.com',
        note: 'Test Alias 2',
        domains: undefined,
      },
      {
        email: 'alias3@brave.com',
        note: 'Test Alias 3',
        domains: undefined,
      },
    ])

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).toBeInTheDocument()
    })
  })

  it('can remove aliases via observer', async () => {
    const service = await setupTest()
    await authenticate(service, AuthenticationStatus.kAuthenticated)

    // Add aliases
    await updateAliases(service, [
      {
        email: 'alias1@brave.com',
        note: 'Test Alias 1',
        domains: undefined,
      },
      {
        email: 'alias2@brave.com',
        note: 'Test Alias 2',
        domains: undefined,
      },
      {
        email: 'alias3@brave.com',
        note: 'Test Alias 3',
        domains: undefined,
      },
    ])

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).toBeInTheDocument()
    })

    // remove first and last alias
    await updateAliases(service, [
      {
        email: 'alias2@brave.com',
        note: 'Test Alias 2',
        domains: undefined,
      },
    ])

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).not.toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).not.toBeInTheDocument()
    })
  })

  it('can update aliases via observer', async () => {
    const service = await setupTest()
    await authenticate(service, AuthenticationStatus.kAuthenticated)

    // Add aliases
    await updateAliases(service, [
      {
        email: 'alias1@brave.com',
        note: 'Test Alias 1',
        domains: undefined,
      },
      {
        email: 'alias2@brave.com',
        note: 'Test Alias 2',
        domains: undefined,
      },
      {
        email: 'alias3@brave.com',
        note: 'Test Alias 3',
        domains: undefined,
      },
    ])

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).toBeInTheDocument()
    })

    // swap first/last alias and rename second alias
    await updateAliases(service, [
      {
        email: 'alias3@brave.com',
        note: 'Test Alias 3',
        domains: undefined,
      },
      {
        email: '2.alias@brave.com',
        note: 'Test Alias 2',
        domains: undefined,
      },
      {
        email: 'alias1@brave.com',
        note: 'Test Alias 1',
        domains: undefined,
      },
    ])

    // Note: We don't actually care about ordering so we don't check that the
    // order of first/last was swapped.
    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).not.toBeInTheDocument()
      expect(screen.queryByText('2.alias@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).toBeInTheDocument()
    })
  })

  it('can clear aliases via observer', async () => {
    const service = await setupTest()
    await authenticate(service, AuthenticationStatus.kAuthenticated)

    // Add aliases
    await updateAliases(service, [
      {
        email: 'alias1@brave.com',
        note: 'Test Alias 1',
        domains: undefined,
      },
      {
        email: 'alias2@brave.com',
        note: 'Test Alias 2',
        domains: undefined,
      },
      {
        email: 'alias3@brave.com',
        note: 'Test Alias 3',
        domains: undefined,
      },
    ])

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).toBeInTheDocument()
    })

    // clear aliases
    await updateAliases(service, [])

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).not.toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).not.toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).not.toBeInTheDocument()
    })
  })

  it('does not show aliases when logged out', async () => {
    const service = await setupTest()

    // Notify of aliases, while not logged in.
    await updateAliases(service, [
      {
        email: 'alias1@brave.com',
        note: 'Test Alias 1',
        domains: undefined,
      },
      {
        email: 'alias2@brave.com',
        note: 'Test Alias 2',
        domains: undefined,
      },
    ])

    // Shouldn't show the aliases in the UI.
    await expectAliasesNotVisible()
  })

  it('hides aliases when logged out', async () => {
    const service = await setupTest()

    await authenticate(service, AuthenticationStatus.kAuthenticated)
    await updateAliases(service)
    await authenticate(service, AuthenticationStatus.kUnauthenticated, '')

    // Shouldn't be showing the aliases in the UI - we're logged out.
    await expectAliasesNotVisible()
  })

  it('does not show aliases which were added before logging in', async () => {
    const service = await setupTest()

    await updateAliases(service)
    await authenticate(service, AuthenticationStatus.kAuthenticated)

    await expectAliasesNotVisible()
  })

  it('does not show aliases from previous logins', async () => {
    const service = await setupTest()

    await authenticate(service, AuthenticationStatus.kAuthenticated)
    await updateAliases(service)

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).toBeInTheDocument()
    })

    await authenticate(service, AuthenticationStatus.kUnauthenticated)
    await authenticate(service, AuthenticationStatus.kAuthenticated)

    // We shouldn't be showing the aliases from the previous login
    await expectAliasesNotVisible()
  })

  it('shows error message when auth fails', async () => {
    const service = await setupTest()

    await authenticate(
      service,
      AuthenticationStatus.kAuthenticating,
      mockEmail,
      'mockErrorMessage',
    )

    await waitFor(() => {
      expect(screen.getByText(/mockErrorMessage/)).toBeInTheDocument()
    })
  })

  it('clears aliases when we are suddenly unauthenticated', async () => {
    const service = await setupTest()

    await authenticate(service, AuthenticationStatus.kAuthenticated)
    await updateAliases(service)
    await authenticate(service, AuthenticationStatus.kUnauthenticated)

    await expectAliasesNotVisible()
  })

  it('can recover from an authentication error', async () => {
    const service = await setupTest()

    await authenticate(
      service,
      AuthenticationStatus.kAuthenticating,
      mockEmail,
      'mockErrorMessage',
    )

    await waitFor(() => {
      expect(screen.queryByText(/mockErrorMessage/)).toBeInTheDocument()
    })

    await authenticate(service, AuthenticationStatus.kAuthenticating, mockEmail)

    await waitFor(() => {
      expect(screen.queryByText(/mockErrorMessage/)).not.toBeInTheDocument()
    })
  })

  it("Data doesn't persist across different logins", async () => {
    const service = await setupTest()

    await authenticate(service, AuthenticationStatus.kAuthenticated)
    await updateAliases(service)
    await authenticate(
      service,
      AuthenticationStatus.kUnauthenticated,
      mockEmail,
    )
    await expectAliasesNotVisible()

    await authenticate(service, AuthenticationStatus.kAuthenticated)
    await expectAliasesNotVisible()
  })

  it("doesn't show an error message if the string is empty", async () => {
    const service = await setupTest()

    await authenticate(
      service,
      AuthenticationStatus.kAuthenticating,
      mockEmail,
      'mockErrorMessage',
    )

    await waitFor(() => {
      expect(screen.queryByText('mockErrorMessage')).toBeInTheDocument()
    })

    await authenticate(
      service,
      AuthenticationStatus.kAuthenticating,
      mockEmail,
      '' /* empty string */,
    )

    await waitFor(() => {
      expect(screen.queryByText('mockErrorMessage')).not.toBeInTheDocument()
    })
  })
})
