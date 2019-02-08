/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import '../../../../app/background/events/windowsEvents'
import actions from '../../../../app/background/actions/windowActions'

interface WindowIdEvent extends chrome.events.Event<(windowId: number) => void> {
  emit: (windowId: number) => void
}

interface WindowReferenceEvent extends chrome.events.Event<(window: chrome.windows.Window) => void> {
  emit: (window: chrome.windows.Window) => void
}

describe('windowsEvents events', () => {
  describe('chrome.windows.onFocusChanged', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(actions, 'windowFocusChanged')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls actions.windowFocusChanged', (cb) => {
      const inputWindowId = 1
      const onFocusChanged = chrome.windows.onFocusChanged as WindowIdEvent
      onFocusChanged.addListener((windowId) => {
        expect(windowId).toBe(inputWindowId)
        expect(spy).toBeCalledWith(inputWindowId)
        cb()
      })
      onFocusChanged.emit(inputWindowId)
    })
  })
  describe('chrome.windows.onCreated', () => {
    let spy: jest.SpyInstance
    const window = {
      id: 1,
      state: 'normal',
      focused: true,
      alwaysOnTop: false,
      incognito: false,
      type: 'normal'
    }
    beforeEach(() => {
      spy = jest.spyOn(actions, 'windowCreated')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls windowCreated', (cb) => {
      const onCreated = chrome.windows.onCreated as WindowReferenceEvent

      onCreated.addListener((windowId) => {
        expect(windowId).toBe(window)
        expect(spy).toBeCalledWith(window)
        cb()
      })
      onCreated.emit(window)
    })
  })

  describe('chrome.windows.onRemoved', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(actions, 'windowRemoved')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls updateShieldsSettings', (cb) => {
      const inputWindowId = 1
      const onRemoved = chrome.windows.onRemoved as WindowIdEvent

      onRemoved.addListener((windowId) => {
        expect(windowId).toBe(inputWindowId)
        expect(spy).toBeCalledWith(inputWindowId)
        cb()
      })
      onRemoved.emit(inputWindowId)
    })
  })
})
