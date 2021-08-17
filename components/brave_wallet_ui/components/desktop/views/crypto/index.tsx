import * as React from 'react'

import { StyledWrapper } from './style'
import {
  TopTabNavTypes,
  AppObjectType,
  AppsListType,
  PriceDataObjectType,
  AccountAssetOptionType,
  RPCTransactionType,
  AssetPriceInfo,
  WalletAccountType,
  AssetPriceTimeframe,
  EthereumChain,
  TokenInfo,
  UpdateAccountNamePayloadType
} from '../../../../constants/types'
import { TopNavOptions } from '../../../../options/top-nav-options'
import { TopTabNav, BackupWarningBanner, AddAccountModal } from '../../'
import { SearchBar, AppList } from '../../../shared'
import locale from '../../../../constants/locale'
import { AppsList } from '../../../../options/apps-list-options'
import { filterAppList } from '../../../../utils/filter-app-list'
import { PortfolioView, AccountsView } from '../'
import {
  HardwareWalletAccount,
  HardwareWalletConnectOpts
} from '../../popup-modals/add-account-modal/hardware-wallet-connect/types'
import * as Result from '../../../../common/types/result'

export interface Props {
  onLockWallet: () => void
  onShowBackup: () => void
  onChangeTimeline: (path: AssetPriceTimeframe) => void
  onSelectAsset: (asset: TokenInfo | undefined) => void
  onCreateAccount: (name: string) => void
  onImportAccount: (accountName: string, privateKey: string) => void
  onConnectHardwareWallet: (opts: HardwareWalletConnectOpts) => Result.Type<HardwareWalletAccount[]>
  onUpdateAccountName: (payload: UpdateAccountNamePayloadType) => { success: boolean }
  onToggleAddModal: () => void
  onUpdateVisibleTokens: (list: string[]) => void
  onSelectNetwork: (network: EthereumChain) => void
  fetchFullTokenList: () => void
  onRemoveAccount: (address: string) => void
  onViewPrivateKey: (address: string, isDefault: boolean) => void
  onDoneViewingPrivateKey: () => void
  privateKey: string
  fullAssetList: TokenInfo[]
  needsBackup: boolean
  accounts: WalletAccountType[]
  networkList: EthereumChain[]
  selectedTimeline: AssetPriceTimeframe
  selectedPortfolioTimeline: AssetPriceTimeframe
  portfolioPriceHistory: PriceDataObjectType[]
  selectedAssetPriceHistory: PriceDataObjectType[]
  selectedUSDAssetPrice: AssetPriceInfo | undefined
  selectedBTCAssetPrice: AssetPriceInfo | undefined
  selectedAsset: TokenInfo | undefined
  portfolioBalance: string
  transactions: (RPCTransactionType | undefined)[]
  userAssetList: AccountAssetOptionType[]
  userWatchList: string[]
  isLoading: boolean
  showAddModal: boolean
  selectedNetwork: EthereumChain
  isFetchingPortfolioPriceHistory: boolean
}

const CryptoView = (props: Props) => {
  const {
    onLockWallet,
    onShowBackup,
    onChangeTimeline,
    onSelectAsset,
    onCreateAccount,
    onConnectHardwareWallet,
    onImportAccount,
    onUpdateAccountName,
    onUpdateVisibleTokens,
    fetchFullTokenList,
    onSelectNetwork,
    onToggleAddModal,
    onRemoveAccount,
    onViewPrivateKey,
    onDoneViewingPrivateKey,
    privateKey,
    selectedNetwork,
    fullAssetList,
    portfolioPriceHistory,
    userAssetList,
    userWatchList,
    selectedTimeline,
    selectedPortfolioTimeline,
    selectedAssetPriceHistory,
    needsBackup,
    accounts,
    networkList,
    selectedAsset,
    portfolioBalance,
    transactions,
    selectedUSDAssetPrice,
    selectedBTCAssetPrice,
    isLoading,
    showAddModal,
    isFetchingPortfolioPriceHistory
  } = props
  const [selectedTab, setSelectedTab] = React.useState<TopTabNavTypes>('portfolio')
  const [favoriteApps, setFavoriteApps] = React.useState<AppObjectType[]>([
    AppsList[0].appList[0]
  ])
  const [filteredAppsList, setFilteredAppsList] = React.useState<AppsListType[]>(AppsList)
  const [hideNav, setHideNav] = React.useState<boolean>(false)
  const [showBackupWarning, setShowBackupWarning] = React.useState<boolean>(needsBackup)

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
    onToggleAddModal()
  }

  const onCloseAddModal = () => {
    onToggleAddModal()
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
        <PortfolioView
          toggleNav={toggleNav}
          accounts={accounts}
          networkList={networkList}
          onChangeTimeline={onChangeTimeline}
          selectedAssetPriceHistory={selectedAssetPriceHistory}
          selectedTimeline={selectedTimeline}
          selectedPortfolioTimeline={selectedPortfolioTimeline}
          onSelectAsset={onSelectAsset}
          onClickAddAccount={onClickAddAccount}
          onSelectNetwork={onSelectNetwork}
          onUpdateVisibleTokens={onUpdateVisibleTokens}
          fetchFullTokenList={fetchFullTokenList}
          selectedAsset={selectedAsset}
          portfolioBalance={portfolioBalance}
          portfolioPriceHistory={portfolioPriceHistory}
          transactions={transactions}
          selectedUSDAssetPrice={selectedUSDAssetPrice}
          selectedBTCAssetPrice={selectedBTCAssetPrice}
          userAssetList={userAssetList}
          isLoading={isLoading}
          selectedNetwork={selectedNetwork}
          fullAssetList={fullAssetList}
          userWatchList={userWatchList}
          isFetchingPortfolioPriceHistory={isFetchingPortfolioPriceHistory}
        />
      }
      {selectedTab === 'accounts' &&
        <AccountsView
          toggleNav={toggleNav}
          accounts={accounts}
          onClickBackup={onShowBackup}
          onClickAddAccount={onClickAddAccount}
          onUpdateAccountName={onUpdateAccountName}
          onRemoveAccount={onRemoveAccount}
          onDoneViewingPrivateKey={onDoneViewingPrivateKey}
          onViewPrivateKey={onViewPrivateKey}
          privateKey={privateKey}
          transactions={transactions}
        />
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
