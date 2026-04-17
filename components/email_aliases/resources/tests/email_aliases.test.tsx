// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen, waitFor, act } from '@testing-library/react'
import { ManagePage } from '../content/email_aliases_manage_page'
import {
  Alias,
  EmailAliasesServiceInterface,
  EmailAliasesServiceObserverInterface,
} from 'gen/brave/components/email_aliases/email_aliases.mojom.m'
import type { AccountState } from 'gen/brave/components/brave_account/mojom/brave_account.mojom.m'
import { isAccountLoggedIn } from '../email_aliases_account_state'

// Mock the email aliases service
class MockEmailAliasesService extends EmailAliasesServiceInterface {
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
    this.observer?.onAliasesUpdated(aliases)
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

function makeLoggedOutAccountState(): AccountState {
  return { loggedOut: {} } as AccountState
}

function makeLoggedInAccountState(email: string): AccountState {
  return { loggedIn: { email } } as AccountState
}

function ManagePageHarness({
  accountState,
  mockService,
}: {
  accountState: AccountState | undefined
  mockService: MockEmailAliasesService
}) {
  const accountStateRef = React.useRef(accountState)
  accountStateRef.current = accountState

  const [aliasesState, setAliasesState] = React.useState<Alias[]>([])

  React.useEffect(() => {
    if (!isAccountLoggedIn(accountState)) {
      setAliasesState([])
    }
  }, [accountState])

  React.useEffect(() => {
    const observer: EmailAliasesServiceObserverInterface = {
      onAliasesUpdated: (aliases: Alias[]) => {
        if (!isAccountLoggedIn(accountStateRef.current)) {
          return
        }
        setAliasesState(aliases)
      },
    }
    return createBindObserver(mockService)(observer)
  }, [mockService])

  return (
    <ManagePage
      accountState={accountState}
      aliasesState={aliasesState}
      emailAliasesService={mockService}
    />
  )
}

type HarnessViewProps = {
  account: AccountState | undefined
  mockService: MockEmailAliasesService
}

const HarnessView = ({ account, mockService }: HarnessViewProps) => (
  <ManagePageHarness
    accountState={account}
    mockService={mockService}
  />
)

// Test setup helpers
const setupTest = async () => {
  const mockEmailAliasesService = new MockEmailAliasesService()
  let account: AccountState | undefined = undefined

  const result = render(
    <HarnessView
      account={account}
      mockService={mockEmailAliasesService}
    />,
  )

  const setAccount = (next: AccountState | undefined) => {
    account = next
    result.rerender(
      <HarnessView
        account={account}
        mockService={mockEmailAliasesService}
      />,
    )
  }

  return { mockEmailAliasesService, setAccount, ...result }
}

const authenticate = async (
  setAccount: (account: AccountState | undefined) => void,
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
})
