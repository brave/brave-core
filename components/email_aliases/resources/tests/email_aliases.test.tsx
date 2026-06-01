// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor, act } from '@testing-library/react'
import { ManagePage } from '../content/email_aliases_manage_page'
import {
  Alias,
  AliasesUpdate,
  EmailAliasesServiceInterface,
  EmailAliasesServiceObserverInterface,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'
import type { AccountState } from 'gen/brave/components/brave_account/mojom/brave_account.mojom.m'
import { useEmailAliases } from '../content/use_email_aliases'
import {
  installMockAuthentication,
  makeLoggedInAccountState,
  makeLoggedOutAccountState,
  restoreMockAuthentication,
} from './mock_authentication'

// Mock the email aliases service
class MockEmailAliasesService implements EmailAliasesServiceInterface {
  private observer?: EmailAliasesServiceObserverInterface

  addObserver(observer: EmailAliasesServiceObserverInterface) {
    if (this.observer !== undefined) {
      throw new Error('Expected observer to be undefined')
    }
    this.observer = observer
  }

  notifyObserverAliasesUpdated(aliases: Alias[]) {
    if (this.observer === undefined) {
      throw new Error('Expected observer to be defined')
    }
    this.observer?.onAliasesUpdated({ aliases } as AliasesUpdate)
  }

  notifyObserverAliasesLoadError(message: string) {
    if (this.observer === undefined) {
      throw new Error('Expected observer to be defined')
    }
    this.observer?.onAliasesUpdated({ error: message } as AliasesUpdate)
  }

  generateAlias = jest.fn()
  updateAlias = jest.fn()
  deleteAlias = jest.fn()
}

const createBindObserver =
  (emailAliasesService: MockEmailAliasesService) =>
  (observer: EmailAliasesServiceObserverInterface) => {
    emailAliasesService.addObserver(observer)
    return () => {}
  }

const mockEmail = 'test@brave.com'

function ManagePageHarness({
  mockService,
}: {
  mockService: MockEmailAliasesService
}) {
  const bindObserver = React.useMemo(
    () => createBindObserver(mockService),
    [mockService],
  )
  const { accountState, aliasesUpdate } = useEmailAliases(bindObserver)

  return (
    <ManagePage
      accountState={accountState}
      aliasesUpdate={aliasesUpdate}
      emailAliasesService={mockService}
    />
  )
}

// Test setup helpers
const setupTest = async () => {
  const mockEmailAliasesService = new MockEmailAliasesService()
  const mockAuth = installMockAuthentication(makeLoggedOutAccountState())

  const result = render(
    <ManagePageHarness mockService={mockEmailAliasesService} />,
  )

  const setAccount = (next: AccountState) => {
    mockAuth.setAccountState(next)
  }

  return { mockEmailAliasesService, setAccount, ...result }
}

const authenticate = async (
  setAccount: (account: AccountState) => void,
  email: string = mockEmail,
) => {
  await act(async () => {
    setAccount(makeLoggedInAccountState(email))
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

const expectAliasesNotVisible = async () => {
  await waitFor(() => {
    expect(screen.queryByText('alias1@brave.com')).not.toBeInTheDocument()
    expect(screen.queryByText('alias2@brave.com')).not.toBeInTheDocument()
  })
}

describe('ManagePage (aliases + Brave Account state)', () => {
  beforeEach(() => {
    jest.clearAllMocks()
  })

  afterEach(() => {
    restoreMockAuthentication()
  })

  it('can add new aliases via observer', async () => {
    const { mockEmailAliasesService, setAccount } = await setupTest()
    await authenticate(setAccount)

    // Add an alias
    await updateAliases(mockEmailAliasesService, [
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
    await updateAliases(mockEmailAliasesService, [
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
    const { mockEmailAliasesService, setAccount } = await setupTest()
    await authenticate(setAccount)

    // Add aliases
    await updateAliases(mockEmailAliasesService, [
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
    await updateAliases(mockEmailAliasesService, [
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
    const { mockEmailAliasesService, setAccount } = await setupTest()
    await authenticate(setAccount)

    // Add aliases
    await updateAliases(mockEmailAliasesService, [
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
    await updateAliases(mockEmailAliasesService, [
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
    const { mockEmailAliasesService, setAccount } = await setupTest()
    await authenticate(setAccount)

    // Add aliases
    await updateAliases(mockEmailAliasesService, [
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
    await updateAliases(mockEmailAliasesService, [])

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).not.toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).not.toBeInTheDocument()
      expect(screen.queryByText('alias3@brave.com')).not.toBeInTheDocument()
    })
  })

  it('does not show aliases when logged out', async () => {
    const { mockEmailAliasesService, setAccount } = await setupTest()
    await act(async () => setAccount(makeLoggedOutAccountState()))

    // Notify of aliases, while not logged in.
    await updateAliases(mockEmailAliasesService, [
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
    const { mockEmailAliasesService, setAccount } = await setupTest()

    await authenticate(setAccount)
    await updateAliases(mockEmailAliasesService)
    await act(async () => setAccount(makeLoggedOutAccountState()))

    // Shouldn't be showing the aliases in the UI - we're logged out.
    await expectAliasesNotVisible()
  })

  it('does not show aliases which were added before logging in', async () => {
    const { mockEmailAliasesService, setAccount } = await setupTest()

    await updateAliases(mockEmailAliasesService)
    await authenticate(setAccount)

    await expectAliasesNotVisible()
  })

  it('does not show aliases from previous logins', async () => {
    const { mockEmailAliasesService, setAccount } = await setupTest()

    await authenticate(setAccount)
    await updateAliases(mockEmailAliasesService)

    await waitFor(() => {
      expect(screen.queryByText('alias1@brave.com')).toBeInTheDocument()
      expect(screen.queryByText('alias2@brave.com')).toBeInTheDocument()
    })

    await act(async () => setAccount(makeLoggedOutAccountState()))
    await authenticate(setAccount)

    // We shouldn't be showing the aliases from the previous login
    await expectAliasesNotVisible()
  })

  it('clears aliases when we are suddenly unauthenticated', async () => {
    const { mockEmailAliasesService, setAccount } = await setupTest()

    await authenticate(setAccount)
    await updateAliases(mockEmailAliasesService)
    await act(async () => setAccount(makeLoggedOutAccountState()))

    await expectAliasesNotVisible()
  })

  it("Data doesn't persist across different logins", async () => {
    const { mockEmailAliasesService, setAccount } = await setupTest()

    await authenticate(setAccount)
    await updateAliases(mockEmailAliasesService)
    await act(async () => setAccount(makeLoggedOutAccountState()))

    await expectAliasesNotVisible()

    await authenticate(setAccount)
    await expectAliasesNotVisible()
  })

  it('shows error alert instead of alias list when refresh fails', async () => {
    const { mockEmailAliasesService, setAccount } = await setupTest()
    await authenticate(setAccount)

    const errorText = 'Could not load your email aliases.'
    await act(() => {
      mockEmailAliasesService.notifyObserverAliasesLoadError(errorText)
    })

    await waitFor(() => {
      expect(screen.getByText(errorText)).toBeInTheDocument()
    })
    expect(screen.queryByText('alias1@brave.com')).not.toBeInTheDocument()
    expect(screen.queryByText('alias2@brave.com')).not.toBeInTheDocument()
  })

  it('clears refresh error alert when user becomes unauthenticated', async () => {
    const { mockEmailAliasesService, setAccount } = await setupTest()
    await authenticate(setAccount)

    const errorText = 'Could not load your email aliases.'
    await act(() => {
      mockEmailAliasesService.notifyObserverAliasesLoadError(errorText)
    })
    await waitFor(() => {
      expect(screen.getByText(errorText)).toBeInTheDocument()
    })

    await act(async () => setAccount(makeLoggedOutAccountState()))

    await waitFor(() => {
      expect(screen.queryByText(errorText)).not.toBeInTheDocument()
    })
  })

  it('does not show refresh error when not authenticated', async () => {
    const { mockEmailAliasesService, setAccount } = await setupTest()
    await act(async () => setAccount(makeLoggedOutAccountState()))

    const errorText = 'Could not load your email aliases.'
    await act(() => {
      mockEmailAliasesService.notifyObserverAliasesLoadError(errorText)
    })

    expect(screen.queryByText(errorText)).not.toBeInTheDocument()
  })

  it('does not display refresh errors or aliases when not authenticated', async () => {
    const { mockEmailAliasesService } = await setupTest()

    const errorText = 'Could not load your email aliases.'
    await act(() => {
      mockEmailAliasesService.notifyObserverAliasesLoadError(errorText)
    })
    await updateAliases(mockEmailAliasesService, [
      {
        email: 'alias1@brave.com',
        note: 'Test Alias 1',
        domains: undefined,
      },
    ])

    expect(screen.queryByText(errorText)).not.toBeInTheDocument()
    await expectAliasesNotVisible()
  })
})
