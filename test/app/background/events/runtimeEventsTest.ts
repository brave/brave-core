/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import '../../../../app/background/events/runtimeEvents'
import windowActions from '../../../../app/background/actions/windowActions'
import tabActions from '../../../../app/background/actions/tabActions'
import { ChromeEvent } from '../../../testData'

interface InputWindows {
  id: number,
  tabs: {
    id: number
  }[]
}

interface RuntimeEvent extends chrome.events.Event<() => void>, ChromeEvent {}

describe('runtimeEvents events', () => {
  describe('chrome.runtime.onStartup listener', () => {
    const inputWindows: InputWindows[] = [
      {
        id: 1,
        tabs: [
          {
            id: 1
          },
          {
            id: 2
          }
        ]
      },
      {
        id: 2,
        tabs: [{
          id: 3
        }]
      }
    ]
    let deferred: (inputWindows: InputWindows[]) => void
    const p = new Promise((resolve, reject) => {
      deferred = resolve
    })
    let windowCreatedSpy: jest.SpyInstance
    let tabCreatedSpy: jest.SpyInstance
    let windowGetAllSpy: jest.SpyInstance
    beforeEach((cb) => {
      windowCreatedSpy = jest.spyOn(windowActions, 'windowCreated')
      tabCreatedSpy = jest.spyOn(tabActions, 'tabCreated')
      windowGetAllSpy = jest.spyOn(chrome.windows, 'getAllAsync').mockImplementation(() => {
        deferred(inputWindows)
        return p
      })

      const event: RuntimeEvent = chrome.runtime.onStartup as RuntimeEvent
      event.addListener(cb)
      event.emit()
    })
    afterEach(() => {
      windowCreatedSpy.mockRestore()
      tabCreatedSpy.mockRestore()
      windowGetAllSpy.mockRestore()
    })
    it('calls windowActions.windowCreated for each window', (cb) => {
      expect.assertions(5)
      p.then((inputWindows) => {
        expect(windowCreatedSpy.mock.calls.length).toBe(2)
        expect(windowCreatedSpy.mock.calls[0].length).toBe(1)
        expect(windowCreatedSpy.mock.calls[0][0]).toBe(inputWindows[0])
        expect(windowCreatedSpy.mock.calls[1].length).toBe(1)
        expect(windowCreatedSpy.mock.calls[1][0]).toBe(inputWindows[1])
        cb()
      })
    })
    it('calls tabActions.tabCreated for each tab in each window', (cb) => {
      expect.assertions(6)
      p.then((inputWindows) => {
        expect(tabCreatedSpy.mock.calls.length).toBe(3)
        expect(tabCreatedSpy.mock.calls[0].length).toBe(1)
        expect(tabCreatedSpy.mock.calls[0][0]).toBe(inputWindows[0].tabs[0])
        expect(tabCreatedSpy.mock.calls[1][0]).toBe(inputWindows[0].tabs[1])
        expect(tabCreatedSpy.mock.calls[2].length).toBe(1)
        expect(tabCreatedSpy.mock.calls[2][0]).toBe(inputWindows[1].tabs[0])
        cb()
      })
    })
  })
})
