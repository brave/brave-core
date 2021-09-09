import * as React from 'react'
import { AddAccountNavTypes, WalletAccountType } from '../../../../constants/types'
import { AddAccountNavOptions } from '../../../../options/add-account-nav-options'
import { Select } from 'brave-ui/components'
import { PopupModal, TopTabNav } from '../..'
import { NavButton } from '../../../extension'
import locale from '../../../../constants/locale'

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

import { HardwareWalletAccount, HardwareWalletConnectOpts } from './hardware-wallet-connect/types'
import HardwareWalletConnect from './hardware-wallet-connect'
import * as Result from '../../../../common/types/result'

export interface Props {
  onClose: () => void
  onCreateAccount: (name: string) => void
  onImportAccount: (accountName: string, privateKey: string) => void
  onImportAccountFromJson: (accountName: string, password: string, json: string) => void
  onConnectHardwareWallet: (opts: HardwareWalletConnectOpts) => Result.Type<HardwareWalletAccount[]>
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
    onImportAccountFromJson,
    onSetImportError
  } = props
  const suggestedAccountName = `${locale.account} ${accounts.length + 1}`
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
        tabList={AddAccountNavOptions}
        onSubmit={onChangeTab}
        selectedTab={tab}
      />

      <StyledWrapper>
        {tab === 'import' &&
          <>
            <DisclaimerWrapper>
              <DisclaimerText>{locale.importAccountDisclaimer}</DisclaimerText>
            </DisclaimerWrapper>
            <SelectWrapper>
              <Select
                value={importOption}
                onChange={onImportOptionChange}
              >
                <div data-value='key'>
                  {locale.importAccountKey}
                </div>
                <div data-value='file'>
                  {locale.importAccountFile}
                </div>
              </Select>
            </SelectWrapper>
            {importError &&
              <ErrorText>{locale.importAccountError}</ErrorText>
            }
            {importOption === 'key' ? (
              <Input
                placeholder={locale.importAccountPlaceholder}
                onChange={handlePrivateKeyChanged}
                type='password'
              />
            ) : (
              <>
                <ImportRow>
                  <ImportButton htmlFor='recoverFile'>{locale.importAccountUploadButton}</ImportButton>
                  <DisclaimerText>{file ? reduceFileName(file[0].name) : locale.importAccountUploadPlaceholder}</DisclaimerText>
                </ImportRow>
                <input
                  type='file'
                  id='recoverFile'
                  name='recoverFile'
                  style={{ display: 'none' }}
                  onChange={onFileUpload}
                />
                <Input
                  placeholder={`Origin ${locale.createPasswordInput}`}
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
              placeholder={locale.addAccountPlaceholder}
              onKeyDown={handleKeyDown}
              onChange={handleAccountNameChanged}
            />
            <NavButton
              onSubmit={onSubmit}
              disabled={isDisabled}
              text={
                tab === 'create' ?
                  `${locale.addAccountCreate} ${locale.account}`
                  : locale.addAccountImport
              }
              buttonType='primary'
            />
          </>
        }
        {tab === 'hardware' && <HardwareWalletConnect onConnectHardwareWallet={onConnectHardwareWallet} />}
      </StyledWrapper>
    </PopupModal>
  )
}

export default AddAccountModal
