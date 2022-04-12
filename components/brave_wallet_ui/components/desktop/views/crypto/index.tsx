import * as React from 'react'
import { Route, useHistory, useLocation, useParams } from 'react-router-dom'
import { StyledWrapper } from './style'
import {
  BraveWallet,
  TopTabNavTypes,
  AccountTransactions,
  WalletAccountType,
  UpdateAccountNamePayloadType,
  WalletRoutes,
  DefaultCurrencies,
  AddAccountNavTypes
} from '../../../../constants/types'
import { TopNavOptions } from '../../../../options/top-nav-options'
import { TopTabNav, WalletBanner, AddAccountModal } from '../../'
import { getLocale } from '../../../../../common/locale'
import { PortfolioView, AccountsView } from '../'
import {
  HardwareWalletConnectOpts
} from '../../popup-modals/add-account-modal/hardware-wallet-connect/types'

interface ParamsType {
  category?: TopTabNavTypes
  id?: string
}

export interface Props {
  onLockWallet: () => void
  onShowBackup: () => void
  onSelectAsset: (asset: BraveWallet.BlockchainToken | undefined) => void
  onCreateAccount: (name: string, coin: BraveWallet.CoinType) => void
  onImportAccount: (accountName: string, privateKey: string, coin: BraveWallet.CoinType) => void
  onImportFilecoinAccount: (accountName: string, key: string, network: string) => void
  onConnectHardwareWallet: (opts: HardwareWalletConnectOpts) => Promise<BraveWallet.HardwareWalletAccount[]>
  onAddHardwareAccounts: (selected: BraveWallet.HardwareWalletAccount[]) => void
  getBalance: (address: string, coin: BraveWallet.CoinType) => Promise<string>
  onUpdateAccountName: (payload: UpdateAccountNamePayloadType) => { success: boolean }
  onShowAddModal: () => void
  onHideAddModal: () => void
  onRemoveAccount: (address: string, hardware: boolean, coin: BraveWallet.CoinType) => void
  onViewPrivateKey: (address: string, isDefault: boolean, coin: BraveWallet.CoinType) => void
  onDoneViewingPrivateKey: () => void
  onImportAccountFromJson: (accountName: string, password: string, json: string) => void
  onSetImportError: (error: boolean) => void
  onOpenWalletSettings: () => void
  defaultCurrencies: DefaultCurrencies
  hasImportError: boolean
  privateKey: string
  userVisibleTokensInfo: BraveWallet.BlockchainToken[]
  needsBackup: boolean
  accounts: WalletAccountType[]
  networkList: BraveWallet.NetworkInfo[]
  transactions: AccountTransactions
  isFilecoinEnabled: boolean
  isSolanaEnabled: boolean
  showAddModal: boolean
  selectedNetwork: BraveWallet.NetworkInfo
  defaultWallet: BraveWallet.DefaultWallet
  isMetaMaskInstalled: boolean
  onShowVisibleAssetsModal: (showModal: boolean) => void
  showVisibleAssetsModal: boolean
}

const CryptoView = (props: Props) => {
  const { pathname: walletLocation } = useLocation()
  let history = useHistory()
  const {
    onLockWallet,
    onShowBackup,
    onSelectAsset,
    onCreateAccount,
    onConnectHardwareWallet,
    onAddHardwareAccounts,
    getBalance,
    onImportAccount,
    onImportFilecoinAccount,
    onUpdateAccountName,
    onRemoveAccount,
    onViewPrivateKey,
    onDoneViewingPrivateKey,
    onImportAccountFromJson,
    onSetImportError,
    onOpenWalletSettings,
    onShowAddModal,
    onHideAddModal,
    onShowVisibleAssetsModal,
    showVisibleAssetsModal,
    defaultCurrencies,
    defaultWallet,
    hasImportError,
    userVisibleTokensInfo,
    privateKey,
    selectedNetwork,
    needsBackup,
    accounts,
    networkList,
    transactions,
    isFilecoinEnabled,
    isSolanaEnabled,
    showAddModal,
    isMetaMaskInstalled
  } = props
  const [hideNav, setHideNav] = React.useState<boolean>(false)
  const [showBackupWarning, setShowBackupWarning] = React.useState<boolean>(needsBackup)
  const [showDefaultWalletBanner, setShowDefaultWalletBanner] = React.useState<boolean>(needsBackup)
  const [selectedAccount, setSelectedAccount] = React.useState<WalletAccountType>()
  const [showMore, setShowMore] = React.useState<boolean>(false)
  const [addAccountModalTab, setAddAccountModalTab] = React.useState<AddAccountNavTypes>('create')

  let { category, id } = useParams<ParamsType>()

  const onSelectTab = (path: TopTabNavTypes) => {
    history.push(`/crypto/${path}`)
  }

  React.useEffect(() => {
    if (category === 'portfolio') {
      if (id !== undefined) {
        if (id === 'add-asset') {
          onShowVisibleAssetsModal(true)
        } else {
          // If the id length is greater than 15 assumes it's a contractAddress
          const asset = id?.length > 15
            ? userVisibleTokensInfo.find((token) => token.contractAddress === id)
            : userVisibleTokensInfo.find((token) => token.symbol.toLowerCase() === id?.toLowerCase())
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
  }, [id, userVisibleTokensInfo, category])

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
    if (walletLocation.includes(WalletRoutes.Accounts)) {
      history.push(WalletRoutes.Accounts)
    }
    onHideAddModal()
  }

  const onClickAddAccount = (tabId: AddAccountNavTypes) => () => {
    setAddAccountModalTab(tabId)
    onShowAddModal()
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

  const onClickSettings = () => {
    chrome.tabs.create({ url: 'chrome://settings/wallet' }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }

  const onClickShowMore = () => {
    setShowMore(true)
  }

  const onClickHideMore = () => {
    if (showMore) {
      setShowMore(false)
    }
  }

  return (
    <StyledWrapper onClick={onClickHideMore}>
      {!hideNav &&
        <>
          <TopTabNav
            selectedTab={category}
            showMore={showMore}
            hasMoreButtons={true}
            onSelectTab={onSelectTab}
            tabList={TopNavOptions()}
            onClickLock={onLockWallet}
            onClickBackup={onShowBackup}
            onClickSettings={onClickSettings}
            onClickMore={onClickShowMore}
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
          toggleNav={toggleNav}
          onClickAddAccount={onClickAddAccount}
          showVisibleAssetsModal={showVisibleAssetsModal}
          onShowVisibleAssetsModal={onShowVisibleAssetsModal}
        />
      </Route>
      <Route path={WalletRoutes.AccountsSub} exact={true}>
        <AccountsView
          defaultCurrencies={defaultCurrencies}
          toggleNav={toggleNav}
          accounts={accounts}
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
          userVisibleTokensInfo={userVisibleTokensInfo}
          networkList={networkList}
        />
      </Route>

      {showAddModal &&
        <AddAccountModal
          accounts={accounts}
          selectedNetwork={selectedNetwork}
          onClose={onCloseAddModal}
          onCreateAccount={onCreateAccount}
          onImportAccount={onImportAccount}
          isFilecoinEnabled={isFilecoinEnabled}
          isSolanaEnabled={isSolanaEnabled}
          onImportFilecoinAccount={onImportFilecoinAccount}
          onConnectHardwareWallet={onConnectHardwareWallet}
          onAddHardwareAccounts={onAddHardwareAccounts}
          getBalance={getBalance}
          onImportAccountFromJson={onImportAccountFromJson}
          hasImportError={hasImportError}
          onSetImportError={onSetImportError}
          tab={addAccountModalTab}
        />
      }
    </StyledWrapper>
  )
}

export default CryptoView
