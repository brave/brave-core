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
