import * as React from 'react'

// Components
import { ConnectWithSite, ConnectedPanel, Panel, WelcomePanel } from '../components/extension'
import { AppList } from '../components/shared'
import { WalletAccountType, PanelTypes, AppObjectType, AppsListType } from '../constants/types'
import { AppsList } from '../options/apps-list-options'
import { filterAppList } from '../utils/filter-app-list'
import LockPanel from '../components/extension/lock-panel'
import {
  StyledExtensionWrapper,
  ScrollContainer
} from './style'

export default {
  title: 'Wallet/Extension/Panels',
  parameters: {
    layout: 'centered'
  },
  argTypes: {
    locked: { control: { type: 'boolean', lock: false } }
  }
}

const accounts: WalletAccountType[] = [
  {
    id: '1',
    name: 'Account 1',
    address: '0x7d66c9ddAED3115d93Bd1790332f3Cd06Cf52B14',
    balance: 0.31178,
    asset: 'eth'
  },
  {
    id: '2',
    name: 'Account 2',
    address: '0x73A29A1da97149722eB09c526E4eAd698895bDCf',
    balance: 0.31178,
    asset: 'eth'
  },
  {
    id: '3',
    name: 'Account 3',
    address: '0x3f29A1da97149722eB09c526E4eAd698895b426',
    balance: 0.31178,
    asset: 'eth'
  }
]

export const _ConnectWithSite = () => {
  const [selectedAccounts, setSelectedAccounts] = React.useState<WalletAccountType[]>([
    accounts[0]
  ])
  const [readyToConnect, setReadyToConnect] = React.useState<boolean>(false)
  const url = 'https://app.uniswap.org'
  const selectAccount = (account: WalletAccountType) => {
    const newList = [...selectedAccounts, account]
    setSelectedAccounts(newList)
  }
  const removeAccount = (account: WalletAccountType) => {
    const newList = selectedAccounts.filter(
      (accounts) => accounts.id !== account.id
    )
    setSelectedAccounts(newList)
  }
  const onSubmit = () => {
    alert(`Connecting to ${url} using: ${JSON.stringify(selectedAccounts)}`)
  }
  const primaryAction = () => {
    if (!readyToConnect) {
      setReadyToConnect(true)
    } else {
      onSubmit()
    }
  }
  const secondaryAction = () => {
    if (readyToConnect) {
      setReadyToConnect(false)
    } else {
      alert('You Clicked The Cancel Button!')
    }
  }
  return (
    <StyledExtensionWrapper>
      <ConnectWithSite
        siteURL={url}
        isReady={readyToConnect}
        accounts={accounts}
        primaryAction={primaryAction}
        secondaryAction={secondaryAction}
        selectAccount={selectAccount}
        removeAccount={removeAccount}
        selectedAccounts={selectedAccounts}
      />
    </StyledExtensionWrapper>
  )
}

_ConnectWithSite.story = {
  name: 'Connect With Site'
}

export const _ConnectedPanel = (args: { locked: boolean }) => {
  const { locked } = args
  const [inputValue, setInputValue] = React.useState<string>('')
  const [walletLocked, setWalletLocked] = React.useState<boolean>(locked)
  const [selectedPanel, setSelectedPanel] = React.useState<PanelTypes>('main')
  const [panelTitle, setPanelTitle] = React.useState<string>('main')
  const [selectedAccount] = React.useState<WalletAccountType>(
    accounts[0]
  )
  const [favoriteApps, setFavoriteApps] = React.useState<AppObjectType[]>([
    AppsList[0].appList[0]
  ])
  const [filteredAppsList, setFilteredAppsList] = React.useState<AppsListType[]>(AppsList)
  const [walletConnected, setWalletConnected] = React.useState<boolean>(true)
  const [hasPasswordError, setHasPasswordError] = React.useState<boolean>(false)
  const toggleConnected = () => {
    setWalletConnected(!walletConnected)
  }

  const getTitle = (path: PanelTypes) => {
    if (path === 'networks') {
      setPanelTitle('Select Network')
    } else {
      setPanelTitle(path)
    }
  }

  const navigateTo = (path: PanelTypes) => {
    if (path === 'expanded') {
      alert('This will expand to main wallet!')
    } else {
      setSelectedPanel(path)
    }
    getTitle(path)
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

  const unlockWallet = () => {
    if (inputValue !== 'password') {
      setHasPasswordError(true)
    } else {
      setWalletLocked(false)
    }
  }

  const handlePasswordChanged = (value: string) => {
    setHasPasswordError(false)
    setInputValue(value)
  }

  return (
    <StyledExtensionWrapper>
      {walletLocked ? (
        <LockPanel
          hasPasswordError={hasPasswordError}
          onSubmit={unlockWallet}
          disabled={inputValue === ''}
          onPasswordChanged={handlePasswordChanged}
        />
      ) : (
        <>
          {selectedPanel === 'main' ? (
            <ConnectedPanel
              selectedAccount={selectedAccount}
              isConnected={walletConnected}
              connectAction={toggleConnected}
              navAction={navigateTo}
            />
          ) : (
            <Panel
              navAction={navigateTo}
              title={panelTitle}
              useSearch={selectedPanel === 'apps'}
              searchAction={selectedPanel === 'apps' ? filterList : undefined}
            >
              {selectedPanel === 'apps' &&
                <ScrollContainer>
                  <AppList
                    list={filteredAppsList}
                    favApps={favoriteApps}
                    addToFav={addToFavorites}
                    removeFromFav={removeFromFavorites}
                    action={browseMore}
                  />
                </ScrollContainer>
              }
            </Panel>
          )}
        </>
      )}
    </StyledExtensionWrapper>
  )
}

_ConnectedPanel.args = {
  locked: false
}

_ConnectedPanel.story = {
  name: 'Connected With Site'
}

export const _SetupWallet = () => {

  const onRestore = () => {
    alert('Will navigate to full wallet restore page')
  }

  const onSetup = () => {
    alert('Will navigate to full wallet onboarding page')
  }

  return (
    <StyledExtensionWrapper>
      <WelcomePanel onRestore={onRestore} onSetup={onSetup} />
    </StyledExtensionWrapper>
  )
}

_SetupWallet.story = {
  name: 'Setup New Wallet'
}
