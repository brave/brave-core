import * as React from 'react'

// Components
import { ConnectWithSite, ConnectedPanel, Panel } from '../components/extension'
import { WalletAccountType, PanelTypes } from '../constants/types'
import { StyledExtensionWrapper, ChildComponentWrapper } from './style'

export default {
  title: 'Wallet/Extension/Panels',
  parameters: {
    layout: 'centered'
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

export const _ConnectedPanel = () => {
  const [selectedPanel, setSelectedPanel] = React.useState<PanelTypes>('main')
  const [panelTitle, setPanelTitle] = React.useState<string>('main')
  const [selectedAccount] = React.useState<WalletAccountType>(
    accounts[0]
  )
  const [walletConnected, setWalletConnected] = React.useState<boolean>(true)

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

  return (
    <StyledExtensionWrapper>
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
        >
          <ChildComponentWrapper>
            <span>Child component for {selectedPanel} panel!</span>
          </ChildComponentWrapper>
        </Panel>
      )
      }
    </StyledExtensionWrapper>
  )
}

_ConnectedPanel.story = {
  name: 'Connected With Site'
}
