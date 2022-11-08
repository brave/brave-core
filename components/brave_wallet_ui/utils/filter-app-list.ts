// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { getLocale } from '../../common/locale'
import { AppsListType } from '../constants/types'

export const filterAppList = (event: any, AppsList: AppsListType[], updateList: (AppsList: AppsListType[]) => void) => {
  const search = event.target.value
  if (search === '') {
    updateList(AppsList)
  } else {
    const mergedList = AppsList.map(category => category.appList).flat()
    const filteredList = mergedList.filter((app) => {
      return (
        app.name.toLowerCase() === search.toLowerCase() ||
        app.name.toLowerCase().startsWith(search.toLowerCase()) ||
        app.name.toLowerCase().includes(search.toLowerCase())
      )
    })
    const newList = [
      {
        category: getLocale('braveWalletSearchCategory'),
        appList: filteredList
      }
    ]
    updateList(newList)
  }
}
