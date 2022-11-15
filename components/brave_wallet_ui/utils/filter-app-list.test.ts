// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { AppsListType } from '../constants/types'
import { filterAppList } from './filter-app-list'
import { mockAppsList } from '../common/constants/mocks'

const mockEvent = {
  target: {
    value: ''
  }
}

const mockUpdateList = jest.fn((appsList: AppsListType[]) => {})

describe('FilterAppList', () => {
  it('should return unfiltered list when search string is empty', () => {
    filterAppList(mockEvent, mockAppsList, mockUpdateList)
    expect(mockUpdateList).toHaveBeenCalledWith(mockAppsList)
  })

  it('should return list with items if name === search term', () => {
    let event = { ...mockEvent, target: { value: mockAppsList[0].appList[0].name } } // name: 'foo'
    const filteredList = [{
      category: 'braveWalletSearchCategory',
      appList: mockAppsList[0].appList
    }]
    filterAppList(event, mockAppsList, mockUpdateList)
    expect(mockUpdateList).toHaveBeenCalledWith(filteredList)
  })

  it('should return list with items if startsWith search term', () => {
    let event = { ...mockEvent, target: { value: 'ba' } } // name: 'bar'
    const filteredList = [{
      category: 'braveWalletSearchCategory',
      appList: mockAppsList[1].appList
    }]
    filterAppList(event, mockAppsList, mockUpdateList)
    expect(mockUpdateList).toHaveBeenCalledWith(filteredList)
  })

  it('should return list with items if includes search term', () => {
    let event = { ...mockEvent, target: { value: 'ar' } } // name: 'bar'
    const filteredList = [{
      category: 'braveWalletSearchCategory',
      appList: mockAppsList[1].appList
    }]
    filterAppList(event, mockAppsList, mockUpdateList)
    expect(mockUpdateList).toHaveBeenCalledWith(filteredList)
  })
})
