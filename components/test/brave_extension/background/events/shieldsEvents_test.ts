/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import '../../../../brave_extension/extension/brave_extension/background/events/shieldsEvents'
import actions from '../../../../brave_extension/extension/brave_extension/background/actions/shieldsPanelActions'
import { blockedResource } from '../../../testData'

describe('shieldsEvents events', () => {
  describe('chrome.braveShields.onBlocked listener', () => {
    let spy: jest.SpyInstance
    beforeEach(() => {
      spy = jest.spyOn(actions, 'resourceBlocked')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('forward details to actions.resourceBlocked', (cb) => {
      chrome.braveShields.onBlocked.addListener((details) => {
        expect(details).toBe(blockedResource)
        expect(spy).toBeCalledWith(details)
        cb()
      })
      chrome.braveShields.onBlocked.emit(blockedResource)
    })
  })
})
