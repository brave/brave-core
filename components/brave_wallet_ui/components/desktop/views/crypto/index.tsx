import * as React from 'react'

import { StyledWrapper } from './style'
import {
  TopTabNavTypes,
  AppObjectType,
  AppsListType,
  PriceDataObjectType,
  AssetOptionType,
  UserAssetOptionType,
  RPCTransactionType,
  AssetPriceReturnInfo,
  WalletAccountType,
  AssetPriceTimeframe
} from '../../../../constants/types'
import { TopNavOptions } from '../../../../options/top-nav-options'
import { TopTabNav, BackupWarningBanner, AddAccountModal } from '../../'
import { SearchBar, AppList } from '../../../shared'
import locale from '../../../../constants/locale'
import { AppsList } from '../../../../options/apps-list-options'
import { filterAppList } from '../../../../utils/filter-app-list'
import { PortfolioView, AccountsView } from '../'

export interface Props {
  onLockWallet: () => void
  onShowBackup: () => void
  onChangeTimeline: (path: AssetPriceTimeframe) => void
  onSelectAsset: (asset: AssetOptionType | undefined) => void
  onCreateAccount: (name: string) => void
  onImportAccount: (name: string, key: string) => void
  onConnectHardwareWallet: (hardware: 'Ledger' | 'Trezor') => void
  needsBackup: boolean
  accounts: WalletAccountType[]
  selectedTimeline: AssetPriceTimeframe
  portfolioPriceHistory: PriceDataObjectType[]
  selectedAssetPriceHistory: PriceDataObjectType[]
  selectedAssetPrice: AssetPriceReturnInfo | undefined
  selectedAsset: AssetOptionType | undefined
  portfolioBalance: string
  transactions: (RPCTransactionType | undefined)[]
  userAssetList: UserAssetOptionType[]
  isLoading: boolean
}

const CryptoView = (props: Props) => {
  const {
    onLockWallet,
    onShowBackup,
    onChangeTimeline,
    onSelectAsset,
    onCreateAccount,
    onImportAccount,
    onConnectHardwareWallet,
    portfolioPriceHistory,
    userAssetList,
    selectedTimeline,
    selectedAssetPriceHistory,
    needsBackup,
    accounts,
    selectedAsset,
    portfolioBalance,
    transactions,
    selectedAssetPrice,
    isLoading
  } = props
  const [selectedTab, setSelectedTab] = React.useState<TopTabNavTypes>('portfolio')
  const [favoriteApps, setFavoriteApps] = React.useState<AppObjectType[]>([
    AppsList[0].appList[0]
  ])
  const [filteredAppsList, setFilteredAppsList] = React.useState<AppsListType[]>(AppsList)
  const [hideNav, setHideNav] = React.useState<boolean>(false)
  const [showBackupWarning, setShowBackupWarning] = React.useState<boolean>(needsBackup)
  const [showAddModal, setShowAddModal] = React.useState<boolean>(false)

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

  const onDismissBackupWarning = () => {
    setShowBackupWarning(false)
  }

  const onClickAddAccount = () => {
    setShowAddModal(true)
  }

  const onCloseAddModal = () => {
    setShowAddModal(false)
  }

  return (
    <StyledWrapper>
      {!hideNav &&
        <>
          <TopTabNav
            tabList={TopNavOptions}
            selectedTab={selectedTab}
            onSubmit={tabTo}
            hasMoreButtons={true}
            onLockWallet={onLockWallet}
          />
          {needsBackup && showBackupWarning &&
            <BackupWarningBanner
              onDismiss={onDismissBackupWarning}
              onBackup={onShowBackup}
            />
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
        <PortfolioView
          toggleNav={toggleNav}
          accounts={accounts}
          onChangeTimeline={onChangeTimeline}
          selectedAssetPriceHistory={selectedAssetPriceHistory}
          selectedTimeline={selectedTimeline}
          onSelectAsset={onSelectAsset}
          onClickAddAccount={onClickAddAccount}
          selectedAsset={selectedAsset}
          portfolioBalance={portfolioBalance}
          portfolioPriceHistory={portfolioPriceHistory}
          transactions={transactions}
          selectedAssetPrice={selectedAssetPrice}
          userAssetList={userAssetList}
          isLoading={isLoading}
        />
      }
      {selectedTab === 'accounts' &&
        <AccountsView
          toggleNav={toggleNav}
          accounts={accounts}
          onClickBackup={onShowBackup}
          onClickAddAccount={onClickAddAccount}
        />
      }
      {selectedTab !== 'portfolio' && selectedTab !== 'defi' && selectedTab !== 'accounts' &&
        <h2>{selectedTab} view</h2>
      }
      {showAddModal &&
        <AddAccountModal
          accounts={accounts}
          title={locale.addAccount}
          onClose={onCloseAddModal}
          onCreateAccount={onCreateAccount}
          onImportAccount={onImportAccount}
          onConnectHardwareWallet={onConnectHardwareWallet}
        />
      }
    </StyledWrapper>
  )
}

export default CryptoView
