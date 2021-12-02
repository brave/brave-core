import * as React from 'react'
import { Route, useHistory, useParams } from 'react-router-dom'
import { StyledWrapper } from './style'
import {
  BraveWallet,
  TopTabNavTypes,
  PriceDataObjectType,
  AccountAssetOptionType,
  AccountTransactions,
  WalletAccountType,
  UpdateAccountNamePayloadType,
  WalletRoutes,
  DefaultCurrencies
} from '../../../../constants/types'
import { TopNavOptions } from '../../../../options/top-nav-options'
import { TopTabNav, WalletBanner, AddAccountModal } from '../../'
import { getLocale } from '../../../../../common/locale'
import { PortfolioView, AccountsView } from '../'
import {
  HardwareWalletConnectOpts
} from '../../popup-modals/add-account-modal/hardware-wallet-connect/types'
import { HardwareWalletAccount } from 'components/brave_wallet_ui/common/hardware/types'

interface ParamsType {
  category?: TopTabNavTypes
  id?: string
}

export interface Props {
  onLockWallet: () => void
  onShowBackup: () => void
  onChangeTimeline: (path: BraveWallet.AssetPriceTimeframe) => void
  onSelectAsset: (asset: BraveWallet.ERCToken | undefined) => void
  onCreateAccount: (name: string) => void
  onImportAccount: (accountName: string, privateKey: string) => void
  onImportFilecoinAccount: (accountName: string, key: string, network: string, protocol: BraveWallet.FilecoinAddressProtocol) => void
  onConnectHardwareWallet: (opts: HardwareWalletConnectOpts) => Promise<HardwareWalletAccount[]>
  onAddHardwareAccounts: (selected: HardwareWalletAccount[]) => void
  getBalance: (address: string) => Promise<string>
  onUpdateAccountName: (payload: UpdateAccountNamePayloadType) => { success: boolean }
  onShowAddModal: () => void
  onHideAddModal: () => void
  onSelectNetwork: (network: BraveWallet.EthereumChain) => void
  onRemoveAccount: (address: string, hardware: boolean) => void
  onViewPrivateKey: (address: string, isDefault: boolean) => void
  onDoneViewingPrivateKey: () => void
  onImportAccountFromJson: (accountName: string, password: string, json: string) => void
  onSetImportError: (error: boolean) => void
  onAddUserAsset: (token: BraveWallet.ERCToken) => void
  onSetUserAssetVisible: (token: BraveWallet.ERCToken, isVisible: boolean) => void
  onRemoveUserAsset: (token: BraveWallet.ERCToken) => void
  onOpenWalletSettings: () => void
  defaultCurrencies: DefaultCurrencies
  addUserAssetError: boolean
  hasImportError: boolean
  transactionSpotPrices: BraveWallet.AssetPrice[]
  privateKey: string
  fullAssetList: BraveWallet.ERCToken[]
  userVisibleTokensInfo: BraveWallet.ERCToken[]
  needsBackup: boolean
  accounts: WalletAccountType[]
  networkList: BraveWallet.EthereumChain[]
  selectedTimeline: BraveWallet.AssetPriceTimeframe
  selectedPortfolioTimeline: BraveWallet.AssetPriceTimeframe
  portfolioPriceHistory: PriceDataObjectType[]
  selectedAssetPriceHistory: PriceDataObjectType[]
  selectedAssetFiatPrice: BraveWallet.AssetPrice | undefined
  selectedAssetCryptoPrice: BraveWallet.AssetPrice | undefined
  selectedAsset: BraveWallet.ERCToken | undefined
  portfolioBalance: string
  transactions: AccountTransactions
  userAssetList: AccountAssetOptionType[]
  isLoading: boolean
  showAddModal: boolean
  selectedNetwork: BraveWallet.EthereumChain
  isFetchingPortfolioPriceHistory: boolean
  defaultWallet: BraveWallet.DefaultWallet
  isMetaMaskInstalled: boolean
  onRetryTransaction: (transaction: BraveWallet.TransactionInfo) => void
  onSpeedupTransaction: (transaction: BraveWallet.TransactionInfo) => void
  onCancelTransaction: (transaction: BraveWallet.TransactionInfo) => void
  onShowVisibleAssetsModal: (showModal: boolean) => void
  showVisibleAssetsModal: boolean
  onFindTokenInfoByContractAddress: (contractAddress: string) => void
  foundTokenInfoByContractAddress?: BraveWallet.ERCToken
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
    onImportFilecoinAccount,
    onUpdateAccountName,
    onSelectNetwork,
    onRemoveAccount,
    onViewPrivateKey,
    onDoneViewingPrivateKey,
    onImportAccountFromJson,
    onSetImportError,
    onAddUserAsset,
    onSetUserAssetVisible,
    onRemoveUserAsset,
    onOpenWalletSettings,
    onShowAddModal,
    onHideAddModal,
    onShowVisibleAssetsModal,
    showVisibleAssetsModal,
    defaultCurrencies,
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
    selectedAssetFiatPrice,
    selectedAssetCryptoPrice,
    isLoading,
    showAddModal,
    isFetchingPortfolioPriceHistory,
    isMetaMaskInstalled,
    onRetryTransaction,
    onCancelTransaction,
    onSpeedupTransaction,
    onFindTokenInfoByContractAddress,
    foundTokenInfoByContractAddress
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
        if (id === 'add-asset') {
          onShowVisibleAssetsModal(true)
        } else {
          const asset = userVisibleTokensInfo.find((token) => token.symbol.toLowerCase() === id?.toLowerCase())
          onSelectAsset(asset)
          setHideNav(true)
        }
      } else {
        onSelectAsset(undefined)
        setHideNav(false)
      }
    }
    if (category === 'accounts') {
      if (id !== undefined) {
        if (id === 'add-account') {
          onShowAddModal()
        } else {
          const account = accounts.find((a) => a.address.toLowerCase() === id?.toLowerCase())
          setSelectedAccount(account)
          setHideNav(true)
        }
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

  const onCloseAddModal = () => {
    history.push(`${WalletRoutes.Accounts}`)
    onHideAddModal()
  }

  const onClickAddAccount = () => {
    history.push(`${WalletRoutes.AddAccountModal}`)
  }

  const onRouteBack = () => {
    history.push(`${WalletRoutes.Accounts}`)
  }

  const selectAsset = (asset: BraveWallet.ERCToken | undefined) => {
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
          {(defaultWallet !== BraveWallet.DefaultWallet.BraveWallet &&
            (defaultWallet !== BraveWallet.DefaultWallet.BraveWalletPreferExtension || (defaultWallet === BraveWallet.DefaultWallet.BraveWalletPreferExtension && isMetaMaskInstalled))) &&
            showDefaultWalletBanner &&
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
          defaultCurrencies={defaultCurrencies}
          toggleNav={toggleNav}
          accounts={accounts}
          networkList={networkList}
          onChangeTimeline={onChangeTimeline}
          selectedAssetPriceHistory={selectedAssetPriceHistory}
          selectedTimeline={selectedTimeline}
          selectedPortfolioTimeline={selectedPortfolioTimeline}
          onSelectAsset={selectAsset}
          onSelectAccount={onSelectAccount}
          onClickAddAccount={onClickAddAccount}
          onSelectNetwork={onSelectNetwork}
          onAddUserAsset={onAddUserAsset}
          onSetUserAssetVisible={onSetUserAssetVisible}
          onRemoveUserAsset={onRemoveUserAsset}
          selectedAsset={selectedAsset}
          portfolioBalance={portfolioBalance}
          portfolioPriceHistory={portfolioPriceHistory}
          transactions={transactions}
          selectedAssetFiatPrice={selectedAssetFiatPrice}
          selectedAssetCryptoPrice={selectedAssetCryptoPrice}
          userAssetList={userAssetList}
          isLoading={isLoading}
          selectedNetwork={selectedNetwork}
          fullAssetList={fullAssetList}
          userVisibleTokensInfo={userVisibleTokensInfo}
          isFetchingPortfolioPriceHistory={isFetchingPortfolioPriceHistory}
          transactionSpotPrices={transactionSpotPrices}
          addUserAssetError={addUserAssetError}
          showVisibleAssetsModal={showVisibleAssetsModal}
          onShowVisibleAssetsModal={onShowVisibleAssetsModal}
          onRetryTransaction={onRetryTransaction}
          onSpeedupTransaction={onSpeedupTransaction}
          onCancelTransaction={onCancelTransaction}
          onFindTokenInfoByContractAddress={onFindTokenInfoByContractAddress}
          foundTokenInfoByContractAddress={foundTokenInfoByContractAddress}
        />
      </Route>
      <Route path={WalletRoutes.AccountsSub} exact={true}>
        <AccountsView
          defaultCurrencies={defaultCurrencies}
          toggleNav={toggleNav}
          accounts={accounts}
          onClickBackup={onShowBackup}
          onClickAddAccount={onClickAddAccount}
          onUpdateAccountName={onUpdateAccountName}
          onRemoveAccount={onRemoveAccount}
          onDoneViewingPrivateKey={onDoneViewingPrivateKey}
          onViewPrivateKey={onViewPrivateKey}
          onSelectAccount={onSelectAccount}
          onSelectAsset={selectAsset}
          goBack={goBack}
          selectedAccount={selectedAccount}
          privateKey={privateKey}
          transactions={transactions}
          selectedNetwork={selectedNetwork}
          transactionSpotPrices={transactionSpotPrices}
          userVisibleTokensInfo={userVisibleTokensInfo}
          onRetryTransaction={onRetryTransaction}
          onSpeedupTransaction={onSpeedupTransaction}
          onCancelTransaction={onCancelTransaction}
        />
      </Route>

      {showAddModal &&
        <AddAccountModal
          accounts={accounts}
          title={getLocale('braveWalletAddAccount')}
          onClose={onCloseAddModal}
          onRouteBackToAccounts={onRouteBack}
          onCreateAccount={onCreateAccount}
          onImportAccount={onImportAccount}
          onImportFilecoinAccount={onImportFilecoinAccount}
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
