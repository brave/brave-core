/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { act, fireEvent, render, screen, waitFor } from '@testing-library/react'

import { mockScenarios, MockScenario } from '../api/mock_scenarios'
import { MockOptions, createTransferApiMock } from '../api/transfer_api_mock'
import { TransferApiProvider } from '../api/transfer_api_context'
import { App, Tab } from './app'

jest.mock('@brave/leo/react/dialog', () => {
  return function MockDialog(
    props: React.PropsWithChildren<{ isOpen?: boolean }>,
  ) {
    return props.isOpen ? <div role='dialog'>{props.children}</div> : null
  }
})

function renderApp(
  scenario: MockScenario,
  tab?: Tab,
  extraOptions: Partial<MockOptions> = {},
) {
  const onHandlerCall = jest.fn()
  const api = createTransferApiMock({
    ...mockScenarios[scenario](),
    ...extraOptions,
    onHandlerCall,
  })
  render(
    <TransferApiProvider {...api}>
      <App initialTab={tab} />
    </TransferApiProvider>,
  )
  return { api, onHandlerCall }
}

type LeoButton = HTMLElement & { isDisabled?: boolean }

// Leo buttons render their label in the light DOM.
function button(label: string, within: ParentNode = document) {
  const element = Array.from(within.querySelectorAll('leo-button')).find(
    (b) => b.textContent === label,
  )
  expect(element).toBeDefined()
  return element as LeoButton
}

// Leo listens for clicks on the native button inside its shadow root.
function click(element: Element) {
  const inner = element.shadowRoot?.querySelector<HTMLElement>('button, a')
  act(() => {
    if (inner) {
      inner.click()
    } else {
      fireEvent.click(element)
    }
  })
}

function deviceRow(name: string) {
  return screen.getByText(name).closest('li') as HTMLElement
}

describe('brave://transfer page', () => {
  describe('devices', () => {
    it('shows an empty state when no devices are found', async () => {
      renderApp('empty')
      expect(
        await screen.findByText('BRAVE_TRANSFER_DEVICES_EMPTY_TITLE'),
      ).toBeInTheDocument()
    })

    it('lists devices that can receive first', async () => {
      renderApp('devices')
      await screen.findByText("James's iPhone")
      const names = Array.from(document.querySelectorAll('.device-name')).map(
        (node) => node.textContent,
      )
      expect(names).toEqual([
        "James's iPhone",
        'iPad Air',
        'Studio Mac mini',
        'Pixel 9',
      ])
      const connected = deviceRow("James's iPhone")
      const offline = deviceRow('Studio Mac mini')
      expect(connected.dataset.connected).toBe('true')
      expect(offline.dataset.connected).toBe('false')
      expect(
        offline.querySelector('leo-button')!.getAttribute('isdisabled')
          ?? (offline.querySelector('leo-button') as any).isDisabled,
      ).toBeTruthy()
    })

    it('shows in-progress transfers with a cancel button', async () => {
      const { onHandlerCall } = renderApp('devices')
      expect(await screen.findByText('Keynote draft.key')).toBeInTheDocument()
      expect(document.querySelector('leo-progressbar')).not.toBeNull()

      const cancel = document.querySelector(
        '[title=BRAVE_TRANSFER_TRANSFER_CANCEL]',
      )!
      click(cancel)
      expect(onHandlerCall).toHaveBeenCalledWith('cancelTransfer', [
        'transfer-1',
      ])
    })

    it('sends files dropped on a device', async () => {
      const { onHandlerCall } = renderApp('devices')
      await screen.findByText("James's iPhone")
      const file = new File(['hello'], 'notes.txt', { type: 'text/plain' })

      await act(async () => {
        fireEvent.drop(deviceRow("James's iPhone"), {
          dataTransfer: { files: [file] },
        })
      })

      await waitFor(() =>
        expect(onHandlerCall).toHaveBeenCalledWith('sendFiles', [
          'iphone',
          [{ name: 'notes.txt', mimeType: 'text/plain', data: [] }],
        ]),
      )
    })

    it('offers drag and drop on desktop', async () => {
      renderApp('devices')
      await screen.findByText("James's iPhone")
      click(deviceRow("James's iPhone").querySelector('leo-button')!)

      expect(
        await screen.findByText('BRAVE_TRANSFER_SEND_DROP_ZONE'),
      ).toBeInTheDocument()
      expect(button('BRAVE_TRANSFER_SEND_CHOOSE_FILES')).toBeInTheDocument()
      expect(
        screen.queryByText('BRAVE_TRANSFER_SEND_CHOOSE_PHOTOS_OR_FILES'),
      ).not.toBeInTheDocument()
    })

    it('uses the native picker on iOS', async () => {
      const { onHandlerCall } = renderApp('devices', 'devices', {
        isIOS: true,
        isMobile: true,
      })
      await screen.findByText("James's iPhone")
      click(deviceRow("James's iPhone").querySelector('leo-button')!)

      click(button('BRAVE_TRANSFER_SEND_CHOOSE_PHOTOS_OR_FILES'))
      expect(
        screen.queryByText('BRAVE_TRANSFER_SEND_DROP_ZONE'),
      ).not.toBeInTheDocument()
      await waitFor(() =>
        expect(onHandlerCall).toHaveBeenCalledWith('pickAndSend', ['iphone']),
      )
    })
  })

  describe('Sync off', () => {
    it('asks the user to turn on Sync', async () => {
      const { onHandlerCall } = renderApp('syncOff')
      expect(
        await screen.findByText('BRAVE_TRANSFER_SYNC_OFF_TEXT'),
      ).toBeInTheDocument()
      // Sync-chain devices cannot connect while Sync is off.
      expect(deviceRow("James's iPhone").dataset.connected).toBe('false')

      click(button('BRAVE_TRANSFER_SYNC_OFF_BUTTON'))
      expect(onHandlerCall).toHaveBeenCalledWith('openSyncSettings', [])
      await waitFor(() =>
        expect(
          screen.queryByText('BRAVE_TRANSFER_SYNC_OFF_TEXT'),
        ).not.toBeInTheDocument(),
      )
    })
  })

  describe('incoming offers', () => {
    it('adds accepted items to the inbox', async () => {
      const { onHandlerCall } = renderApp('incomingOffer', 'inbox')
      expect(await screen.findByText('IMG_2041.HEIC')).toBeInTheDocument()
      expect(
        await screen.findByText('BRAVE_TRANSFER_INBOX_EMPTY_TITLE'),
      ).toBeInTheDocument()

      click(button('BRAVE_TRANSFER_OFFER_ACCEPT'))

      await waitFor(() =>
        expect(onHandlerCall).toHaveBeenCalledWith('respondToOffer', [
          'offer-1',
          true,
        ]),
      )
      await waitFor(() =>
        expect(
          screen.queryByText('BRAVE_TRANSFER_OFFERS_TITLE'),
        ).not.toBeInTheDocument(),
      )
      expect(await screen.findByText('Boarding pass.pdf')).toBeInTheDocument()
      expect(document.querySelectorAll('.row:not(.header-row)')).toHaveLength(2)
    })

    it('declines without adding to the inbox', async () => {
      const { onHandlerCall } = renderApp('incomingOffer', 'inbox')
      await screen.findByText('IMG_2041.HEIC')

      click(button('BRAVE_TRANSFER_OFFER_DECLINE'))

      await waitFor(() =>
        expect(onHandlerCall).toHaveBeenCalledWith('respondToOffer', [
          'offer-1',
          false,
        ]),
      )
      await waitFor(() =>
        expect(screen.queryByText('IMG_2041.HEIC')).not.toBeInTheDocument(),
      )
      expect(
        screen.getByText('BRAVE_TRANSFER_INBOX_EMPTY_TITLE'),
      ).toBeInTheDocument()
    })
  })

  describe('inbox', () => {
    beforeEach(() => {
      Object.assign(navigator, { clipboard: { writeText: jest.fn() } })
    })

    it('shows text, links, images and files', async () => {
      renderApp('inbox', 'inbox')
      expect(await screen.findByText('IMG_2038.jpeg')).toBeInTheDocument()
      expect(screen.getByText('Q3 roadmap.pdf')).toBeInTheDocument()
      expect(
        screen.getByText(
          'https://github.com/brave/brave-browser/wiki/Brave-Transfer',
        ),
      ).toBeInTheDocument()
      expect(document.querySelector('.text')).toHaveAttribute(
        'data-collapsed',
        'true',
      )
      expect(document.querySelectorAll('.row:not(.header-row)')).toHaveLength(5)
    })

    it('opens files, folders and links through the host', async () => {
      const { onHandlerCall } = renderApp('inbox', 'inbox')
      const row = (await screen.findByText('Q3 roadmap.pdf')).closest('.row')!
      click(button('BRAVE_TRANSFER_OPEN', row))
      await waitFor(() =>
        expect(onHandlerCall).toHaveBeenCalledWith('openItem', ['inbox-4']),
      )

      click(button('BRAVE_TRANSFER_SHOW_IN_FOLDER', row))
      await waitFor(() =>
        expect(onHandlerCall).toHaveBeenCalledWith('showItemInFolder', [
          'inbox-4',
        ]),
      )

      fireEvent.click(
        screen.getByText(
          'https://github.com/brave/brave-browser/wiki/Brave-Transfer',
        ),
      )
      await waitFor(() =>
        expect(onHandlerCall).toHaveBeenCalledWith('openLink', ['inbox-1']),
      )
    })

    it('copies links and text', async () => {
      renderApp('inbox', 'inbox')
      await screen.findByText('IMG_2038.jpeg')
      const linkRow = screen
        .getByText('https://github.com/brave/brave-browser/wiki/Brave-Transfer')
        .closest('.row')!
      await act(async () => {
        click(button('BRAVE_TRANSFER_COPY', linkRow))
      })
      expect(navigator.clipboard.writeText).toHaveBeenCalledWith(
        'https://github.com/brave/brave-browser/wiki/Brave-Transfer',
      )
      expect(linkRow).toHaveTextContent('BRAVE_TRANSFER_COPIED')
    })

    it('marks files that were moved or deleted', async () => {
      renderApp('inbox', 'inbox')
      const row = (await screen.findByText('Screen Recording.mov')).closest(
        '.row',
      )!
      expect(row).toHaveTextContent('BRAVE_TRANSFER_FILE_MISSING')
      const open = button('BRAVE_TRANSFER_OPEN', row)
      expect(open.isDisabled ?? open.getAttribute('isdisabled')).toBeTruthy()
    })

    it('loads older items in pages', async () => {
      const { onHandlerCall } = renderApp('inbox', 'inbox')
      await screen.findByText('IMG_2038.jpeg')
      expect(onHandlerCall).toHaveBeenCalledWith('getInbox', [0, 100])

      click(button('BRAVE_TRANSFER_LOAD_MORE'))
      await waitFor(() =>
        expect(onHandlerCall).toHaveBeenCalledWith('getInbox', [0, 200]),
      )
    })

    it('clears history only after confirmation', async () => {
      const { onHandlerCall } = renderApp('inbox', 'inbox')
      await screen.findByText('IMG_2038.jpeg')

      click(button('BRAVE_TRANSFER_CLEAR_HISTORY'))
      expect(
        screen.getByText('BRAVE_TRANSFER_CLEAR_HISTORY_CONFIRM_TEXT'),
      ).toBeInTheDocument()
      click(button('BRAVE_TRANSFER_CANCEL'))
      expect(screen.queryByRole('dialog')).not.toBeInTheDocument()
      expect(onHandlerCall).not.toHaveBeenCalledWith('clearHistory', [])

      click(button('BRAVE_TRANSFER_CLEAR_HISTORY'))
      const dialog = screen.getByRole('dialog')
      click(button('BRAVE_TRANSFER_CLEAR_HISTORY', dialog))
      await waitFor(() =>
        expect(onHandlerCall).toHaveBeenCalledWith('clearHistory', []),
      )
      expect(
        await screen.findByText('BRAVE_TRANSFER_INBOX_EMPTY_TITLE'),
      ).toBeInTheDocument()
    })
  })

  describe('settings', () => {
    function toggle(label: string) {
      return screen
        .getByText(label)
        .closest('.setting')!
        .querySelector('leo-toggle') as HTMLElement & { checked?: boolean }
    }

    it('turns auto-accept off by default and saves to the downloads folder', async () => {
      renderApp('settings', 'settings')
      expect(
        await screen.findByText(/\(\/Users\/james\/Downloads\)/),
      ).toBeInTheDocument()
      expect(toggle('BRAVE_TRANSFER_SETTINGS_AUTO_ACCEPT').checked).toBe(false)
      expect(toggle('BRAVE_TRANSFER_SETTINGS_AUTO_TRUST').checked).toBe(true)
      expect(
        screen.queryByText('BRAVE_TRANSFER_SETTINGS_SAVE_LOCATION_RESET'),
      ).not.toBeInTheDocument()
    })

    it('changes the save location', async () => {
      const { onHandlerCall } = renderApp('settings', 'settings')
      await screen.findByText(/\(\/Users\/james\/Downloads\)/)

      click(button('BRAVE_TRANSFER_SETTINGS_SAVE_LOCATION_CHANGE'))
      await waitFor(() =>
        expect(onHandlerCall).toHaveBeenCalledWith('chooseSaveDirectory', []),
      )
      expect(
        await screen.findByText('/Users/james/Desktop/Transfers'),
      ).toBeInTheDocument()

      click(button('BRAVE_TRANSFER_SETTINGS_SAVE_LOCATION_RESET'))
      await waitFor(() =>
        expect(onHandlerCall).toHaveBeenCalledWith('updateSettings', [
          expect.objectContaining({ saveDirectory: '' }),
        ]),
      )
      expect(
        await screen.findByText(/\(\/Users\/james\/Downloads\)/),
      ).toBeInTheDocument()
    })

    it('shows per-device overrides and removes trusted devices', async () => {
      const { onHandlerCall } = renderApp('settings', 'settings')
      await screen.findByText('Pixel 9')
      const values = Array.from(
        document.querySelectorAll('.trusted-device [slot=value]'),
      ).map((node) => node.textContent)
      expect(values).toEqual([
        'BRAVE_TRANSFER_POLICY_DEFAULT',
        'BRAVE_TRANSFER_POLICY_ALWAYS_ACCEPT',
        'BRAVE_TRANSFER_POLICY_ALWAYS_ASK',
        'BRAVE_TRANSFER_POLICY_BLOCK',
      ])
      expect(
        screen.getAllByText('BRAVE_TRANSFER_KEY_FINGERPRINT'),
      ).toHaveLength(3)

      const pixel = screen.getByText('Pixel 9').closest('li')!
      expect(pixel).toHaveTextContent('BRAVE_TRANSFER_NEVER_CONNECTED')
      click(button('BRAVE_TRANSFER_REMOVE_DEVICE', pixel))
      await waitFor(() =>
        expect(onHandlerCall).toHaveBeenCalledWith('removeDevice', ['pixel']),
      )
      await waitFor(() =>
        expect(screen.queryByText('Pixel 9')).not.toBeInTheDocument(),
      )
    })

    it('shows read-only destinations and permissions on iOS', async () => {
      const { onHandlerCall } = renderApp('iosPermissions', 'settings')
      expect(
        await screen.findByText(
          'BRAVE_TRANSFER_SETTINGS_SAVE_LOCATION_IOS_FILES',
        ),
      ).toBeInTheDocument()
      expect(
        screen.queryByText('BRAVE_TRANSFER_SETTINGS_SAVE_LOCATION_CHANGE'),
      ).not.toBeInTheDocument()

      expect(
        await screen.findByText('BRAVE_TRANSFER_PERMISSION_DENIED'),
      ).toBeInTheDocument()
      click(button('BRAVE_TRANSFER_PERMISSIONS_OPEN_SETTINGS'))
      expect(onHandlerCall).toHaveBeenCalledWith('openSystemSettings', [])

      click(button('BRAVE_TRANSFER_PERMISSIONS_ALLOW'))
      await waitFor(() =>
        expect(onHandlerCall).toHaveBeenCalledWith('requestPermissions', []),
      )
      await waitFor(() =>
        expect(
          screen.queryByText('BRAVE_TRANSFER_PERMISSIONS_ALLOW'),
        ).not.toBeInTheDocument(),
      )
    })
  })
})
