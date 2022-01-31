import * as React from 'react'

import {
  BraveWallet,
  AddAccountNavTypes,
  WalletAccountType,
  CreateAccountOptionsType
} from '../../../../constants/types'
import { Select } from 'brave-ui/components'
import { PopupModal } from '../..'
import { NavButton, DividerLine } from '../../../extension'
import { getLocale } from '../../../../../common/locale'
import AccountTypeItem from './account-type-item'
import { CreateAccountOptions } from '../../../../options/create-account-options'
// Styled Components
import {
  Input,
  StyledWrapper,
  DisclaimerText,
  DisclaimerWrapper,
  SelectWrapper,
  ImportButton,
  ImportRow,
  ErrorText,
  SelectAccountTypeWrapper,
  SelectAccountTitle,
  SelectAccountItemWrapper
} from './style'

import { HardwareWalletConnectOpts } from './hardware-wallet-connect/types'
import HardwareWalletConnect from './hardware-wallet-connect'
import { FilecoinNetwork } from '../../../../common/hardware/types'

export interface Props {
  onClose: () => void
  onCreateAccount: (name: string, coin: BraveWallet.CoinType) => void
  onImportAccount: (accountName: string, privateKey: string) => void
  isFilecoinEnabled: boolean
  onImportFilecoinAccount: (accountName: string, key: string, network: FilecoinNetwork, protocol: BraveWallet.FilecoinAddressProtocol) => void
  onImportAccountFromJson: (accountName: string, password: string, json: string) => void
  onConnectHardwareWallet: (opts: HardwareWalletConnectOpts) => Promise<BraveWallet.HardwareWalletAccount[]>
  onAddHardwareAccounts: (selected: BraveWallet.HardwareWalletAccount[]) => void
  getBalance: (address: string) => Promise<string>
  onSetImportError: (hasError: boolean) => void
  onRouteBackToAccounts: () => void
  hasImportError: boolean
  accounts: WalletAccountType[]
  tab: AddAccountNavTypes
  selectedNetwork: BraveWallet.EthereumChain
}

const AddAccountModal = (props: Props) => {
  const {
    accounts,
    selectedNetwork,
    isFilecoinEnabled,
    hasImportError,
    tab,
    onClose,
    onCreateAccount,
    onImportAccount,
    onImportFilecoinAccount,
    onConnectHardwareWallet,
    onAddHardwareAccounts,
    getBalance,
    onImportAccountFromJson,
    onSetImportError,
    onRouteBackToAccounts
  } = props
  const suggestedAccountName = `${getLocale('braveWalletAccount')} ${accounts.length + 1}`
  const [importOption, setImportOption] = React.useState<string>('key')
  const [file, setFile] = React.useState<HTMLInputElement['files']>()
  const [accountName, setAccountName] = React.useState<string>(suggestedAccountName)
  const [privateKey, setPrivateKey] = React.useState<string>('')
  const [password, setPassword] = React.useState<string>('')
  const [selectedAccountType, setSelectedAccountType] = React.useState<CreateAccountOptionsType | undefined>(undefined)
  const passwordInputRef = React.useRef<HTMLInputElement>(null)

  const importError = React.useMemo(() => {
    return hasImportError
  }, [hasImportError])

  const onClickClose = () => {
    setPassword('')
    setPrivateKey('')
    setFile(undefined)
    onSetImportError(false)
    onClose()
  }

  const handleAccountNameChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    setAccountName(event.target.value)
    onSetImportError(false)
  }

  const handlePrivateKeyChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    setPrivateKey(event.target.value)
    onSetImportError(false)
  }

  const handlePasswordChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    setPassword(event.target.value)
    onSetImportError(false)
  }

  const onClickCreateAccount = () => {
    if (tab === 'create') {
      onCreateAccount(accountName, selectedAccountType?.coin || BraveWallet.CoinType.ETH)
      onRouteBackToAccounts()
      return
    }
    if (tab === 'import') {
      if (importOption === 'key') {
        if (selectedAccountType?.coin === BraveWallet.CoinType.FIL) {
          onImportFilecoinAccount(accountName, privateKey, BraveWallet.FILECOIN_TESTNET, BraveWallet.FilecoinAddressProtocol.BLS)
        } else {
          onImportAccount(accountName, privateKey)
        }
      } else {
        if (file) {
          const index = file[0]
          const reader = new FileReader()
          reader.onload = function () {
            if (reader.result) {
              onImportAccountFromJson(accountName, password, (reader.result.toString().trim()))
            }
          }
          reader.readAsText(index)
        }
      }
    }
  }

  const handleKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter') {
      onClickCreateAccount()
    }
  }

  const onImportOptionChange = (value: string) => {
    setImportOption(value)
  }

  const reduceFileName = (address: string) => {
    const firstHalf = address.slice(0, 4)
    const secondHalf = address.slice(-4)
    const reduced = firstHalf.concat('......', secondHalf)
    return reduced
  }

  const onFileUpload = (file: React.ChangeEvent<HTMLInputElement>) => {
    if (file.target.files) {
      setFile(file.target.files)
      onSetImportError(false)
      passwordInputRef.current?.focus()
    }
  }

  const isDisabled = React.useMemo(() => {
    if (tab === 'create') {
      return accountName === ''
    }
    if (tab === 'import') {
      if (importOption === 'key') {
        return accountName === '' || privateKey === ''
      }
      return accountName === '' || file === undefined
    }
    return false
  }, [tab, importOption, accountName, privateKey, file])

  const modalTitle = React.useMemo((): string => {
    switch (tab) {
      case 'create':
        // Will need different logic here to determine how many accounts a user has for each network.
        setAccountName(selectedAccountType?.name + ' ' + suggestedAccountName)
        return selectedAccountType
          ? getLocale('braveWalletCreateAccount').replace('$1', selectedAccountType.name)
          : getLocale('braveWalletCreateAccountButton')
      case 'import':
        setAccountName('')
        return selectedAccountType
          ? getLocale('braveWalletCreateAccountImportAccount').replace('$1', selectedAccountType.name)
          : getLocale('braveWalletAddAccountImport')
      case 'hardware':
        setAccountName('')
        return getLocale('braveWalletAddAccountImportHardware')
      default:
        return ''
    }
  }, [tab, selectedAccountType])

  const onSelectAccountType = (accountType: CreateAccountOptionsType) => () => {
    setSelectedAccountType(accountType)
  }

  const buttonText = React.useMemo((): string => {
    switch (tab) {
      case 'create':
        return getLocale('braveWalletAddAccountCreate')
      case 'import':
        return getLocale('braveWalletAddAccountImport')
      case 'hardware':
        return getLocale('braveWalletAddAccountConnect')
      default:
        return ''
    }
  }, [tab])

  return (
    <PopupModal title={modalTitle} onClose={onClickClose}>
      <DividerLine />
      {selectedAccountType ? (
        <StyledWrapper>
          {tab === 'import' &&
            <>
              <DisclaimerWrapper>
                <DisclaimerText>{getLocale('braveWalletImportAccountDisclaimer')}</DisclaimerText>
              </DisclaimerWrapper>
              <SelectWrapper>
                <Select
                  value={importOption}
                  onChange={onImportOptionChange}
                >
                  <div data-value='key'>
                    {getLocale('braveWalletImportAccountKey')}
                  </div>
                  <div data-value='file'>
                    {getLocale('braveWalletImportAccountFile')}
                  </div>
                </Select>
              </SelectWrapper>
              {importError &&
                <ErrorText>{getLocale('braveWalletImportAccountError')}</ErrorText>
              }
              {importOption === 'key' ? (
                <Input
                  placeholder={getLocale('braveWalletImportAccountPlaceholder')}
                  onChange={handlePrivateKeyChanged}
                  type='password'
                  autoFocus={true}
                />
              ) : (
                <>
                  <ImportRow>
                    <ImportButton htmlFor='recoverFile'>{getLocale('braveWalletImportAccountUploadButton')}</ImportButton>
                    <DisclaimerText>{file ? reduceFileName(file[0].name) : getLocale('braveWalletImportAccountUploadPlaceholder')}</DisclaimerText>
                  </ImportRow>
                  <input
                    type='file'
                    id='recoverFile'
                    name='recoverFile'
                    style={{ display: 'none' }}
                    onChange={onFileUpload}
                  />
                  <Input
                    placeholder={`Origin ${getLocale('braveWalletCreatePasswordInput')}`}
                    onChange={handlePasswordChanged}
                    type='password'
                    ref={passwordInputRef}
                  />
                </>
              )}
            </>
          }
          {tab !== 'hardware' &&
            <>
              <Input
                value={accountName}
                placeholder={getLocale('braveWalletAddAccountPlaceholder')}
                onKeyDown={handleKeyDown}
                onChange={handleAccountNameChanged}
                autoFocus={true}
              />
              <NavButton
                onSubmit={onClickCreateAccount}
                disabled={isDisabled}
                text={
                  tab === 'create'
                    ? getLocale('braveWalletCreateAccountButton')
                    : getLocale('braveWalletAddAccountImport')
                }
                buttonType='primary'
              />
            </>
          }
          {tab === 'hardware' &&
            <HardwareWalletConnect
              onConnectHardwareWallet={onConnectHardwareWallet}
              onAddHardwareAccounts={onAddHardwareAccounts}
              getBalance={getBalance}
              selectedAccountType={selectedAccountType}
              preAddedHardwareWalletAccounts={
                accounts.filter(account => ['Ledger', 'Trezor'].includes(account.accountType))
              }
              selectedNetwork={selectedNetwork}
            />
          }
        </StyledWrapper>
      ) : (
        <SelectAccountTypeWrapper>
          <SelectAccountTitle>{getLocale('braveWalletCreateAccountTitle')}</SelectAccountTitle>
          <DividerLine />
          {CreateAccountOptions(isFilecoinEnabled).map((network) =>
            <SelectAccountItemWrapper key={network.coin}>
              <AccountTypeItem
                onClickCreate={onSelectAccountType(network)}
                icon={network.icon}
                description={network.description}
                title={network.name}
                buttonText={buttonText}
              />
              {network.coin !== BraveWallet.CoinType.FIL &&
                <DividerLine />
              }
            </SelectAccountItemWrapper>
          )}
        </SelectAccountTypeWrapper>
      )}
    </PopupModal>
  )
}

export default AddAccountModal
