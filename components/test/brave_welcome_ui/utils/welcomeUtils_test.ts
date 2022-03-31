/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { getValidBrowserProfiles, getSelectedBrowserProfile, getSourceBrowserProfileIndex, isValidBrowserProfiles } from '../../../brave_welcome_ui/welcomeUtils'
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

  describe('getSelectedBrowserProfile tests', () => {
    it('should return the selected browser profile with the given index', () => {
      const expected = mockImportSources[0]
      const result = getSelectedBrowserProfile('1', mockImportSources)
      expect(result).toEqual(expected)
    })
  })

  describe('getSourceBrowserProfileIndex tests', () => {
    it('should return the index of the selected browser profile in the component state', () => {
      const mockComponentState = {
        selectedBrowserProfile: mockImportSources[0]
      }
      const expected = 1
      const result = getSourceBrowserProfileIndex(mockComponentState)
      expect(result).toEqual(expected)
    })

    it('should return "0" given an empty state', () => {
      const mockComponentState = null
      const expected = 0
      const result = getSourceBrowserProfileIndex(mockComponentState)
      expect(result).toEqual(expected)
    })
  })

  describe('isValidBrowserProfiles tests', () => {
    it('should return truthy if valid browserProfile data', () => {
      const result = isValidBrowserProfiles(mockImportSources)
      expect(result).toBeTruthy()
    })

    it('should return falsy if invalid data', () => {
      const mockBrowserProfiles = null
      const result = isValidBrowserProfiles(mockBrowserProfiles)
      expect(result).toBeFalsy()
    })
  })
})
