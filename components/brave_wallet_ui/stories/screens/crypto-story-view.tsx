// Copyright (c) 2020 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import { StyledWrapper } from '../../components/desktop/views/crypto/style'
import {
  TopTabNavTypes,
  BraveWallet,
  AppsListType
} from '../../constants/types'
import { TopNavOptions } from '../../options/top-nav-options'
import { TopTabNav, WalletBanner, AddAccountModal } from '../../components/desktop'
import { SearchBar, AppList } from '../../components/shared'
import { getLocale } from '../../../common/locale'
import { AppsList } from '../../options/apps-list-options'
import { filterAppList } from '../../utils/filter-app-list'
import { Accounts, PortfolioOverview } from '../../components/desktop/views'
import WalletPageStory from '../wrappers/wallet-page-story-wrapper'

export interface Props {
  showAddModal: boolean
  needsBackup: boolean
  onShowBackup: () => void
}

const CryptoStoryView = (props: Props) => {
  const {
    needsBackup,
    showAddModal,
    onShowBackup
  } = props
  const [showBackupWarning, setShowBackupWarning] = React.useState<boolean>(needsBackup)
  const [showDefaultWalletBanner, setShowDefaultWalletBanner] = React.useState<boolean>(needsBackup)
  const [hideNav] = React.useState<boolean>(false)
  const [filteredAppsList, setFilteredAppsList] = React.useState<AppsListType[]>(AppsList())
  const [favoriteApps, setFavoriteApps] = React.useState<BraveWallet.AppItem[]>([
    AppsList()[0].appList[0]
  ])
  const [selectedTab, setSelectedTab] = React.useState<TopTabNavTypes>('portfolio')
  const [showMore, setShowMore] = React.useState<boolean>(false)

  const browseMore = () => {
    alert('Will expand to view more!')
  }

  const onSelectTab = (path: TopTabNavTypes) => {
    setSelectedTab(path)
  }

  const addToFavorites = (app: BraveWallet.AppItem) => {
    const newList = [...favoriteApps, app]
    setFavoriteApps(newList)
  }
  const removeFromFavorites = (app: BraveWallet.AppItem) => {
    const newList = favoriteApps.filter(
      (fav) => fav.name !== app.name
    )
    setFavoriteApps(newList)
  }

  const filterList = (event: any) => {
    filterAppList(event, AppsList(), setFilteredAppsList)
  }

  const onDismissBackupWarning = () => {
    setShowBackupWarning(false)
  }

  const onDismissDefaultWalletBanner = () => {
    setShowDefaultWalletBanner(false)
  }

  const onClickSettings = () => {
    // Does nothing in storybook
    alert('Will Nav to brave://settings/wallet')
  }

  const onClickMore = () => {
    setShowMore(true)
  }

  const onHideMore = () => {
    if (showMore) {
      setShowMore(false)
    }
  }

  return (
    <WalletPageStory>
      <StyledWrapper onClick={onHideMore}>
        {!hideNav &&
          <>
            <TopTabNav
              tabList={TopNavOptions()}
              selectedTab={selectedTab}
              onSelectTab={onSelectTab}
              hasMoreButtons={true}
              showMore={showMore}
              onClickBackup={onShowBackup}
              onClickMore={onClickMore}
              onClickSettings={onClickSettings}
            />
            {showDefaultWalletBanner &&
              <WalletBanner
                description={getLocale('braveWalletDefaultWalletBanner')}
                onDismiss={onDismissDefaultWalletBanner}
                onClick={onClickSettings}
                bannerType='warning'
                buttonText={getLocale('braveWalletWalletPopupSettings')}
              />
            }

            {needsBackup && showBackupWarning &&
              <WalletBanner
                description={getLocale('braveWalletBackupWarningText')}
                onDismiss={onDismissBackupWarning}
                onClick={onShowBackup}
                bannerType='danger'
                buttonText={getLocale('braveWalletBackupButton')}
              />
            }
          </>
        }
        {selectedTab === 'apps' &&
          <>
            <SearchBar
              placeholder={getLocale('braveWalletSearchText')}
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
          <PortfolioOverview />
        }
        {selectedTab === 'accounts' &&
          <Accounts />
        }
        {showAddModal &&
          <AddAccountModal />
        }
      </StyledWrapper>
    </WalletPageStory>
  )
}

export default CryptoStoryView
