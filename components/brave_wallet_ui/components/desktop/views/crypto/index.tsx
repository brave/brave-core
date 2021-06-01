import * as React from 'react'

import { StyledWrapper } from './style'
import { TopTabNavTypes, AppObjectType, AppsListType } from '../../../../constants/types'
import { TopNavOptions } from '../../../../options/top-nav-options'
import { TopTabNav, WalletMorePopup, BackupWarningBanner } from '../../'
import { SearchBar, AppList } from '../../../shared'
import locale from '../../../../constants/locale'
import { AppsList } from '../../../../options/apps-list-options'
import { filterAppList } from '../../../../utils/filter-app-list'
import { PortfolioView } from '../'

export interface Props {
  onLockWallet: () => void
  onShowBackup: () => void
  needsBackup: boolean
}

const CryptoView = (props: Props) => {
  const { onLockWallet, onShowBackup, needsBackup } = props
  const [selectedTab, setSelectedTab] = React.useState<TopTabNavTypes>('portfolio')
  const [favoriteApps, setFavoriteApps] = React.useState<AppObjectType[]>([
    AppsList[0].appList[0]
  ])
  const [filteredAppsList, setFilteredAppsList] = React.useState<AppsListType[]>(AppsList)
  const [hideNav, setHideNav] = React.useState<boolean>(false)
  const [showBackupWarning, setShowBackupWarning] = React.useState<boolean>(needsBackup)
  const [showPopup, setShowPopup] = React.useState<boolean>(false)

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

  const onShowPopup = () => {
    setShowPopup(true)
  }

  const onHidePopup = () => {
    if (showPopup) {
      setShowPopup(false)
    }
  }

  const onShowSettings = () => {
    alert('Will Show Settings')
  }

  const onDismissBackupWarning = () => {
    setShowBackupWarning(false)
  }

  return (
    <StyledWrapper onClick={onHidePopup}>
      {!hideNav &&
        <>
          <TopTabNav
            tabList={TopNavOptions}
            selectedTab={selectedTab}
            onSubmit={tabTo}
            hasMoreButton={true}
            onClickMoreButton={onShowPopup}
          />
          {needsBackup && showBackupWarning &&
            <BackupWarningBanner onDismiss={onDismissBackupWarning} />
          }
        </>
      }
      {selectedTab === 'defi' &&
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
      {selectedTab !== 'portfolio' && selectedTab !== 'defi' &&
        <h2>{selectedTab} view</h2>
      }
      {showPopup &&
        <WalletMorePopup
          onClickLock={onLockWallet}
          onClickSetting={onShowSettings}
          onClickBackup={onShowBackup}
        />
      }
    </StyledWrapper>
  )
}

export default CryptoView
