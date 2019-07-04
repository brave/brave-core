/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { getValidBrowserProfiles } from '../../../brave_welcome_ui/welcomeUtils'
import { mockImportSources } from '../../testData'

describe('welcome utils tests', () => {
  describe('getfilterBrowserProfiles tests', () => {
    it('should filter out profiles with "safari" and "Bookmarks HTML File" as names', () => {
      const result = getValidBrowserProfiles(mockImportSources)
      // Chrome browser profile object
      const expected = [mockImportSources[0]]
      expect(result).toEqual(expected)
    })
  })
})
