/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as localeAPI from '../../../../brave_extension/extension/brave_extension/background/api/localeAPI'

describe('locale API', () => {
  describe('getMessage', () => {
    let spy: jest.SpyInstance
    const message = 'NESPRESS YOURSELF'
    beforeEach(() => {
      spy = jest.spyOn(chrome.i18n, 'getMessage')
    })
    afterEach(() => {
      spy.mockRestore()
    })
    it('calls chrome.i18n.getMessage with the message', () => {
      localeAPI.getLocale(message)
      expect(spy).toBeCalledTimes(1)
      expect(spy.mock.calls[0][0]).toBe(message)
    })
  })
})
