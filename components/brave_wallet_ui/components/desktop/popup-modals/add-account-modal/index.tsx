import * as React from 'react'
import { AddAccountNavTypes, WalletAccountType, HardwareWalletAccount } from '../../../../constants/types'
import { AddAccountNavOptions } from '../../../../options/add-account-nav-options'
import { Select } from 'brave-ui/components'
import { PopupModal, TopTabNav } from '../..'
import { NavButton } from '../../../extension'
import { getLocale } from '../../../../../common/locale'

// Styled Components
import {
  Input,
  StyledWrapper,
  DisclaimerText,
  DisclaimerWrapper,
  SelectWrapper,
  ImportButton,
  ImportRow,
  ErrorText
} from './style'

import { HardwareWalletConnectOpts } from './hardware-wallet-connect/types'
import HardwareWalletConnect from './hardware-wallet-connect'

export interface Props {
  onClose: () => void
  onCreateAccount: (name: string) => void
  onImportAccount: (accountName: string, privateKey: string) => void
  onImportAccountFromJson: (accountName: string, password: string, json: string) => void
  onConnectHardwareWallet: (opts: HardwareWalletConnectOpts) => Promise<HardwareWalletAccount[]>
  onAddHardwareAccounts: (selected: HardwareWalletAccount[]) => void
  getBalance: (address: string) => Promise<string>
  onSetImportError: (hasError: boolean) => void
  hasImportError: boolean
  accounts: WalletAccountType[]
  title: string
}

const AddAccountModal = (props: Props) => {
  const {
    title,
    accounts,
    hasImportError,
    onClose,
    onCreateAccount,
    onImportAccount,
    onConnectHardwareWallet,
    onAddHardwareAccounts,
    getBalance,
    onImportAccountFromJson,
    onSetImportError
  } = props
  const suggestedAccountName = `${getLocale('braveWalletAccount')} ${accounts.length + 1}`
  const [tab, setTab] = React.useState<AddAccountNavTypes>('create')
  const [importOption, setImportOption] = React.useState<string>('key')
  const [file, setFile] = React.useState<HTMLInputElement['files']>()
  const [accountName, setAccountName] = React.useState<string>(suggestedAccountName)
  const [privateKey, setPrivateKey] = React.useState<string>('')
  const [password, setPassword] = React.useState<string>('')

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

  const onSubmit = () => {
    if (tab === 'create') {
      onCreateAccount(accountName)
      return
    }
    if (tab === 'import') {
      if (importOption === 'key') {
        onImportAccount(accountName, privateKey)
        return
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
        return
      }
    }
  }

  const handleKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter') {
      onSubmit()
    }
  }

  const onChangeTab = (id: AddAccountNavTypes) => {
    if (id === 'create') {
      setAccountName(suggestedAccountName)
    } else {
      setAccountName('')
    }
    setTab(id)
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

  return (
    <PopupModal title={title} onClose={onClickClose}>
      <TopTabNav
        tabList={AddAccountNavOptions()}
        onSubmit={onChangeTab}
        selectedTab={tab}
      />

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
            />
            <NavButton
              onSubmit={onSubmit}
              disabled={isDisabled}
              text={
                tab === 'create' ?
                  `${getLocale('braveWalletAddAccountCreate')} ${getLocale('braveWalletAccount')}`
                  : getLocale('braveWalletAddAccountImport')
              }
              buttonType='primary'
            />
          </>
        }
        {tab === 'hardware' && <HardwareWalletConnect onConnectHardwareWallet={onConnectHardwareWallet} onAddHardwareAccounts={onAddHardwareAccounts} getBalance={getBalance} />}
      </StyledWrapper>
    </PopupModal>
  )
}

export default AddAccountModal
