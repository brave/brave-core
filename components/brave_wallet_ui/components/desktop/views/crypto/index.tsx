import * as React from 'react'

import { StyledWrapper } from './style'
import { TopTabNavTypes, AppObjectType, AppsListType } from '../../../../constants/types'
import { TopNavOptions } from '../../../../options/top-nav-options'
import { TopTabNav } from '../../'
import { SearchBar, AppList } from '../../../shared'
import locale from '../../../../constants/locale'
import { AppsList } from '../../../../options/apps-list-options'
import { filterAppList } from '../../../../utils/filter-app-list'
import { PortfolioView } from '../'

const CryptoView = () => {
  const [selectedTab, setSelectedTab] = React.useState<TopTabNavTypes>('portfolio')
  const [favoriteApps, setFavoriteApps] = React.useState<AppObjectType[]>([
    AppsList[0].appList[0]
  ])
  const [filteredAppsList, setFilteredAppsList] = React.useState<AppsListType[]>(AppsList)
  const [hideNav, setHideNav] = React.useState<boolean>(false)

  // In the future these will be actual paths
  // for example wallet/crypto/portfolio
  const tabTo = (path: TopTabNavTypes) => {
    setSelectedTab(path)
  }

  const browseMore = () => {
    alert('Will expand to view more!')
  }

  const addToFavorites = (app: AppObjectType) => {
    const newList = [...favoriteApps, app]
    setFavoriteApps(newList)
  }
  const removeFromFavorites = (app: AppObjectType) => {
    const newList = favoriteApps.filter(
      (fav) => fav.name !== app.name
    )
    setFavoriteApps(newList)
  }

  const filterList = (event: any) => {
    filterAppList(event, AppsList, setFilteredAppsList)
  }

  const toggleNav = () => {
    setHideNav(!hideNav)
  }

  return (
    <StyledWrapper>
      {!hideNav &&
        <TopTabNav
          tabList={TopNavOptions}
          selectedTab={selectedTab}
          onSubmit={tabTo}
        />
      }
      {selectedTab === 'apps' &&
        <>
          <SearchBar
            placeholder={locale.searchText}
            action={filterList}
          />
          <AppList
            list={filteredAppsList}
            favApps={favoriteApps}
            addToFav={addToFavorites}
            removeFromFav={removeFromFavorites}
            action={browseMore}
          />
        </>
      }
      {selectedTab === 'portfolio' &&
        <PortfolioView toggleNav={toggleNav} />
      }
      {selectedTab !== 'portfolio' && selectedTab !== 'apps' &&
        <h2>{selectedTab} view</h2>
      }
    </StyledWrapper>
  )
}

export default CryptoView
