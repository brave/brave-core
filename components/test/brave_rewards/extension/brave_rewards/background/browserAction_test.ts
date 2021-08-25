/* global describe, it */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import {
  setBadgeText
} from '../../../../../brave_rewards/resources/extension/brave_rewards/background/browserAction'

describe('Rewards Panel extension - Browser Action', () => {
  describe('setBadgeText', () => {
    let spyText: jest.SpyInstance
    let spyColor: jest.SpyInstance

    beforeEach(() => {
      spyText = jest.spyOn(chrome.browserAction, 'setBadgeText')
      spyColor = jest.spyOn(chrome.browserAction, 'setBadgeBackgroundColor')
    })

    afterEach(() => {
      spyText.mockRestore()
      spyColor.mockRestore()
    })

    it('publisher is not verified, no pending notifications', () => {
      const state: RewardsExtension.State = {
        notifications: {}
      }

      setBadgeText(state)

      expect(spyText).toHaveBeenCalled()
      expect(spyText.mock.calls[0][0]).toEqual({
        text: '',
        tabId: undefined
      })
    })

    it('publisher is not verified, pending notifications', () => {
      const state: RewardsExtension.State = {
        notifications: {
          '1': {
            id: 'test'
          }
        }
      }

      setBadgeText(state)

      expect(spyText).toHaveBeenCalled()
      expect(spyText.mock.calls[0][0]).toEqual({
        text: '1',
        tabId: undefined
      })
    })

    it('publisher is verified, no pending notifications', () => {
      const state: RewardsExtension.State = {
        notifications: {}
      }

      setBadgeText(state, true, 1)

      expect(spyText).toHaveBeenCalled()
      const data = spyText.mock.calls[0][0]
      expect(data.tabId).toEqual(1)
      expect(data.text).toEqual('✓️')

      expect(spyColor).toHaveBeenCalled()
      expect(spyColor.mock.calls[0][0]).toEqual({
        color: '#4C54D2',
        tabId: 1
      })
    })

    it('publisher is verified, pending notifications', () => {
      const state: RewardsExtension.State = {
        notifications: {
          '1': {
            id: 'test'
          }
        }
      }

      setBadgeText(state, true, 1)

      expect(spyText).toHaveBeenCalled()
      expect(spyText.mock.calls[0][0]).toEqual({
        text: '1',
        tabId: 1
      })

      expect(spyColor).toHaveBeenCalled()
      expect(spyColor.mock.calls[0][0]).toEqual({
        color: '#FB542B',
        tabId: 1
      })
    })

    it('publisher is not verified with tabId, pending notifications', () => {
      const state: RewardsExtension.State = {
        notifications: {
          '1': {
            id: 'test'
          }
        }
      }

      setBadgeText(state, false, 1)

      expect(spyText).toHaveBeenCalled()
      expect(spyText.mock.calls[0][0]).toEqual({
        text: '1',
        tabId: 1
      })

      expect(spyColor).toHaveBeenCalled()
      expect(spyColor.mock.calls[0][0]).toEqual({
        color: '#FB542B',
        tabId: 1
      })
    })
  })
})
