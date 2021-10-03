import * as React from 'react'
import { Route, useHistory, useParams } from 'react-router-dom'
import { StyledWrapper } from './style'
import {
  TopTabNavTypes,
  PriceDataObjectType,
  AccountAssetOptionType,
  TransactionListInfo,
  AssetPriceInfo,
  WalletAccountType,
  AssetPriceTimeframe,
  EthereumChain,
  TokenInfo,
  UpdateAccountNamePayloadType,
  WalletRoutes,
  DefaultWallet,
  HardwareWalletAccount
} from '../../../../constants/types'
import { TopNavOptions } from '../../../../options/top-nav-options'
import { TopTabNav, WalletBanner, AddAccountModal } from '../../'
import { getLocale } from '../../../../../common/locale'
import { PortfolioView, AccountsView } from '../'
import {
  HardwareWalletConnectOpts
} from '../../popup-modals/add-account-modal/hardware-wallet-connect/types'

interface ParamsType {
  category: TopTabNavTypes
  id: string
}

export interface Props {
  onLockWallet: () => void
  onShowBackup: () => void
  onChangeTimeline: (path: AssetPriceTimeframe) => void
  onSelectAsset: (asset: TokenInfo | undefined) => void
  onCreateAccount: (name: string) => void
  onImportAccount: (accountName: string, privateKey: string) => void
  onConnectHardwareWallet: (opts: HardwareWalletConnectOpts) => Promise<HardwareWalletAccount[]>
  onAddHardwareAccounts: (selected: HardwareWalletAccount[]) => void
  getBalance: (address: string) => Promise<string>
  onUpdateAccountName: (payload: UpdateAccountNamePayloadType) => { success: boolean }
  onToggleAddModal: () => void
  onSelectNetwork: (network: EthereumChain) => void
  fetchFullTokenList: () => void
  onRemoveAccount: (address: string, hardware: boolean) => void
  onViewPrivateKey: (address: string, isDefault: boolean) => void
  onDoneViewingPrivateKey: () => void
  onImportAccountFromJson: (accountName: string, password: string, json: string) => void
  onSetImportError: (hasError: boolean) => void
  onAddUserAsset: (token: TokenInfo) => void
  onSetUserAssetVisible: (contractAddress: string, isVisible: boolean) => void
  onRemoveUserAsset: (contractAddress: string) => void
  onOpenWalletSettings: () => void
  addUserAssetError: boolean
  hasImportError: boolean
  transactionSpotPrices: AssetPriceInfo[]
  privateKey: string
  fullAssetList: TokenInfo[]
  userVisibleTokensInfo: TokenInfo[]
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
  transactions: (TransactionListInfo | undefined)[]
  userAssetList: AccountAssetOptionType[]
  isLoading: boolean
  showAddModal: boolean
  selectedNetwork: EthereumChain
  isFetchingPortfolioPriceHistory: boolean
  defaultWallet: DefaultWallet
}

const CryptoView = (props: Props) => {
  let history = useHistory()
  const {
    onLockWallet,
    onShowBackup,
    onChangeTimeline,
    onSelectAsset,
    onCreateAccount,
    onConnectHardwareWallet,
    onAddHardwareAccounts,
    getBalance,
    onImportAccount,
    onUpdateAccountName,
    fetchFullTokenList,
    onSelectNetwork,
    onToggleAddModal,
    onRemoveAccount,
    onViewPrivateKey,
    onDoneViewingPrivateKey,
    onImportAccountFromJson,
    onSetImportError,
    onAddUserAsset,
    onSetUserAssetVisible,
    onRemoveUserAsset,
    onOpenWalletSettings,
    defaultWallet,
    addUserAssetError,
    hasImportError,
    userVisibleTokensInfo,
    transactionSpotPrices,
    privateKey,
    selectedNetwork,
    fullAssetList,
    portfolioPriceHistory,
    userAssetList,
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
  const [hideNav, setHideNav] = React.useState<boolean>(false)
  const [showBackupWarning, setShowBackupWarning] = React.useState<boolean>(needsBackup)
  const [showDefaultWalletBanner, setShowDefaultWalletBanner] = React.useState<boolean>(needsBackup)
  const [selectedAccount, setSelectedAccount] = React.useState<WalletAccountType>()

  let { category, id } = useParams<ParamsType>()

  const tabTo = (path: TopTabNavTypes) => {
    history.push(`/crypto/${path}`)
  }

  React.useEffect(() => {
    if (category === 'portfolio') {
      if (id !== undefined) {
        const asset = userVisibleTokensInfo.find((token) => token.symbol.toLowerCase() === id.toLowerCase())
        onSelectAsset(asset)
        setHideNav(true)
      } else {
        onSelectAsset(undefined)
        setHideNav(false)
      }
    }
    if (category === 'accounts') {
      if (id !== undefined) {
        const account = accounts.find((a) => a.address.toLowerCase() === id.toLowerCase())
        setSelectedAccount(account)
        setHideNav(true)
      } else {
        setSelectedAccount(undefined)
        setHideNav(false)
      }
    }
  }, [id, userVisibleTokensInfo, category, accounts])

  const toggleNav = () => {
    setHideNav(!hideNav)
  }

  const onDismissBackupWarning = () => {
    setShowBackupWarning(false)
  }

  const onDismissDefaultWalletBanner = () => {
    setShowDefaultWalletBanner(false)
  }

  const onClickAddAccount = () => {
    onToggleAddModal()
  }

  const onCloseAddModal = () => {
    onToggleAddModal()
  }

  const selectAsset = (asset: TokenInfo | undefined) => {
    if (asset) {
      history.push(`${WalletRoutes.Portfolio}/${asset.symbol}`)
    } else {
      onSelectAsset(asset)
      history.push(WalletRoutes.Portfolio)
    }
  }

  const goBack = () => {
    setSelectedAccount(undefined)
    history.push(WalletRoutes.Accounts)
    setHideNav(false)
  }

  const onSelectAccount = (account: WalletAccountType | undefined) => {
    if (account) {
      history.push(`${WalletRoutes.Accounts}/${account.address}`)
    }
  }

  return (
    <StyledWrapper>
      {!hideNav &&
        <>
          <TopTabNav
            tabList={TopNavOptions()}
            selectedTab={category}
            onSubmit={tabTo}
            hasMoreButtons={true}
            onLockWallet={onLockWallet}
          />
          {defaultWallet !== DefaultWallet.BraveWallet && showDefaultWalletBanner &&
            <WalletBanner
              onDismiss={onDismissDefaultWalletBanner}
              onClick={onOpenWalletSettings}
              bannerType='warning'
              buttonText={getLocale('braveWalletWalletPopupSettings')}
              description={getLocale('braveWalletDefaultWalletBanner')}
            />
          }
          {needsBackup && showBackupWarning &&
            <WalletBanner
              onDismiss={onDismissBackupWarning}
              onClick={onShowBackup}
              bannerType='danger'
              buttonText={getLocale('braveWalletBackupButton')}
              description={getLocale('braveWalletBackupWarningText')}
            />
          }
        </>
      }

      <Route path={WalletRoutes.PortfolioSub} exact={true}>
        <PortfolioView
          toggleNav={toggleNav}
          accounts={accounts}
          networkList={networkList}
          onChangeTimeline={onChangeTimeline}
          selectedAssetPriceHistory={selectedAssetPriceHistory}
          selectedTimeline={selectedTimeline}
          selectedPortfolioTimeline={selectedPortfolioTimeline}
          onSelectAsset={selectAsset}
          onClickAddAccount={onClickAddAccount}
          onSelectNetwork={onSelectNetwork}
          fetchFullTokenList={fetchFullTokenList}
          onAddUserAsset={onAddUserAsset}
          onSetUserAssetVisible={onSetUserAssetVisible}
          onRemoveUserAsset={onRemoveUserAsset}
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
          userVisibleTokensInfo={userVisibleTokensInfo}
          isFetchingPortfolioPriceHistory={isFetchingPortfolioPriceHistory}
          transactionSpotPrices={transactionSpotPrices}
          addUserAssetError={addUserAssetError}
        />
      </Route>
      <Route path={WalletRoutes.AccountsSub} exact={true}>
        <AccountsView
          toggleNav={toggleNav}
          accounts={accounts}
          onClickBackup={onShowBackup}
          onClickAddAccount={onClickAddAccount}
          onUpdateAccountName={onUpdateAccountName}
          onRemoveAccount={onRemoveAccount}
          onDoneViewingPrivateKey={onDoneViewingPrivateKey}
          onViewPrivateKey={onViewPrivateKey}
          onSelectAccount={onSelectAccount}
          goBack={goBack}
          selectedAccount={selectedAccount}
          privateKey={privateKey}
          transactions={transactions}
          selectedNetwork={selectedNetwork}
          transactionSpotPrices={transactionSpotPrices}
          userVisibleTokensInfo={userVisibleTokensInfo}
        />
      </Route>

      {showAddModal &&
        <AddAccountModal
          accounts={accounts}
          title={getLocale('braveWalletAddAccount')}
          onClose={onCloseAddModal}
          onCreateAccount={onCreateAccount}
          onImportAccount={onImportAccount}
          onConnectHardwareWallet={onConnectHardwareWallet}
          onAddHardwareAccounts={onAddHardwareAccounts}
          getBalance={getBalance}
          onImportAccountFromJson={onImportAccountFromJson}
          hasImportError={hasImportError}
          onSetImportError={onSetImportError}
        />
      }
    </StyledWrapper>
  )
}

export default CryptoView
