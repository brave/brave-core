/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as backgroundAPI from '../../../brave_new_tab_ui/api/background'
import { images as backgrounds } from '../../../brave_new_tab_ui/data/backgrounds'

describe('new tab background api tests', () => {
  describe('randomBackgroundImage', () => {
    it('grabs a random image included in the background image object', () => {
      const assertion = backgroundAPI.randomBackgroundImage()
      expect(backgrounds).toContainEqual(assertion)
    })
  })
})
