// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { useDispatch } from 'react-redux'
import { useHistory, useParams } from 'react-router'

// utils
import { FILECOIN_FORMAT_DESCRIPTION_URL } from '../../../../common/constants/urls'
import { getLocale, getLocaleWithTag } from '$web-common/locale'
import { copyToClipboard } from '../../../../utils/copy-to-clipboard'

// options
import { CreateAccountOptions } from '../../../../options/create-account-options'

// types
import {
  BraveWallet,
  CreateAccountOptionsType,
  WalletRoutes,
  ImportAccountErrorType,
  FilecoinNetwork,
  FilecoinNetworkTypes,
  FilecoinNetworkLocaleMapping
} from '../../../../constants/types'

// actions
import { WalletActions } from '../../../../common/actions'
import { PanelActions } from '../../../../panel/actions'

// components
import { Select } from 'brave-ui/components'
import { NavButton } from '../../../extension/buttons/nav-button/index'
import { DividerLine } from '../../../extension/divider/index'
import { PopupModal } from '../index'
import { SelectAccountType } from './select-account-type/select-account-type'

// style
import {
  DisclaimerText,
  ErrorText,
  ImportButton,
  ImportDisclaimer,
  ImportRow,
  Input,
  SelectWrapper,
  StyledWrapper
} from './style'

import {
  WarningText,
  WarningWrapper
} from '../account-settings-modal/account-settings-modal.style'

// selectors
import {
  UISelectors,
  WalletSelectors
} from '../../../../common/selectors'

// hooks
import {
  useSafeUISelector,
  useSafeWalletSelector
} from '../../../../common/hooks/use-safe-selector'

interface Params {
  accountTypeName: string
}

const reduceFileName = (address: string) => {
  const firstHalf = address.slice(0, 4)
  const secondHalf = address.slice(-4)
  const reduced = firstHalf.concat('......', secondHalf)
  return reduced
}

export const ImportAccountModal = () => {
  // refs
  const passwordInputRef = React.useRef<HTMLInputElement>(null)

  // routing
  const history = useHistory()
  const { accountTypeName } = useParams<Params>()

  // redux
  const isFilecoinEnabled = useSafeWalletSelector(WalletSelectors.isFilecoinEnabled)
  const isSolanaEnabled = useSafeWalletSelector(WalletSelectors.isSolanaEnabled)
  const hasImportError =
    useSafeWalletSelector(WalletSelectors.importAccountError)

  // memos
  const createAccountOptions = React.useMemo(() => {
    return CreateAccountOptions({
      isFilecoinEnabled,
      isSolanaEnabled,
      isBitcoinEnabled: false // No bitcoin imported accounts by now.
    })
  }, [isFilecoinEnabled, isSolanaEnabled])

  const selectedAccountType = React.useMemo(() => {
    return createAccountOptions.find((option) => {
      return option.name.toLowerCase() === accountTypeName?.toLowerCase()
    })
  }, [accountTypeName, createAccountOptions])

  const isPanel = useSafeUISelector(UISelectors.isPanel)

  // state
  const [accountName, setAccountName] = React.useState<string>('')
  const [filecoinNetwork, setFilecoinNetwork] = React.useState<FilecoinNetwork>('f')
  const [importOption, setImportOption] = React.useState<string>('key')
  const [privateKey, setPrivateKey] = React.useState<string>('')
  const [file, setFile] = React.useState<HTMLInputElement['files']>()
  const [password, setPassword] = React.useState<string>('')

  // redux
  const dispatch = useDispatch()

  // methods
  const setImportError = React.useCallback((hasError: ImportAccountErrorType) => {
    dispatch(WalletActions.setImportAccountError(hasError))
  }, [])

  const onClickClose = React.useCallback(() => {
    setImportError(undefined)
    history.push(WalletRoutes.Accounts)
  }, [setImportError])

  const importAccount = React.useCallback((accountName: string, privateKey: string, coin: BraveWallet.CoinType) => {
    dispatch(WalletActions.importAccount({ accountName, privateKey, coin }))
  }, [])

  const importFilecoinAccount = React.useCallback(
    (
      accountName: string,
      privateKey: string,
      coin: BraveWallet.CoinType,
      network: FilecoinNetwork
    ) => {
      dispatch(
        WalletActions
          .importAccount({ accountName, privateKey, coin, network })
      )
    }, [])

  const importAccountFromJson = React.useCallback((accountName: string, password: string, json: string) => {
    dispatch(
      WalletActions
        .importAccountFromJson({ accountName, password, json }))
  }, [])

  const handleAccountNameChanged = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setAccountName(event.target.value)
    setImportError(undefined)
  }, [setImportError])

  const onChangeFilecoinNetwork = React.useCallback((network: FilecoinNetwork) => {
    setFilecoinNetwork(network)
  }, [])

  const handlePrivateKeyChanged = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setPrivateKey(event.target.value)
    setImportError(undefined)
  }, [setImportError])

  const onClearClipboard = React.useCallback(() => {
    copyToClipboard('')
  }, [])

  const onClickFileUpload = () => {
    // To prevent panel from being closed when file chooser is open
    if (isPanel) {
      dispatch(PanelActions.setCloseOnDeactivate(false))
      // For resume close on deactive when file chooser is close(select/cancel)
      window.addEventListener('focus', onFocusFileUpload)
    }
  }

  const onFocusFileUpload = () => {
    if (isPanel) {
      dispatch(PanelActions.setCloseOnDeactivate(true))
      window.removeEventListener('focus', onFocusFileUpload)
    }
  }

  const onFileUpload = React.useCallback((file: React.ChangeEvent<HTMLInputElement>) => {
    if (file.target.files) {
      setFile(file.target.files)
      setImportError(undefined)
      passwordInputRef.current?.focus()
    }
  }, [setImportError])

  const handlePasswordChanged = React.useCallback((event: React.ChangeEvent<HTMLInputElement>) => {
    setPassword(event.target.value)
    setImportError(undefined)
  }, [setImportError])

  const onClickCreateAccount = React.useCallback(() => {
    if (importOption === 'key') {
      if (selectedAccountType?.coin === BraveWallet.CoinType.FIL) {
        importFilecoinAccount(
          accountName,
          privateKey,
          BraveWallet.CoinType.FIL,
          filecoinNetwork
        )
      } else {
        importAccount(accountName, privateKey, selectedAccountType?.coin || BraveWallet.CoinType.ETH)
      }
      return
    }

    if (file) {
      const index = file[0]
      const reader = new FileReader()
      reader.onload = function () {
        if (reader.result) {
          importAccountFromJson(accountName, password, (reader.result.toString().trim()))
        }
      }

      reader.readAsText(index)
    }
  }, [
    importOption,
    selectedAccountType,
    accountName,
    privateKey,
    file,
    password,
    filecoinNetwork
  ])

  const handleKeyDown = React.useCallback((event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter') {
      onClickCreateAccount()
    }
  }, [onClickCreateAccount])

  const onSelectAccountType = React.useCallback((accountType: CreateAccountOptionsType) => () => {
    history.push(WalletRoutes.ImportAccountModal.replace(':accountTypeName?', accountType.name.toLowerCase()))
  }, [])

  React.useEffect(() => {
    if (hasImportError === false) {
      setImportError(undefined)
      history.push(WalletRoutes.Accounts)
    }
  }, [hasImportError, setImportError])

  // computed
  const isDisabled = accountName === ''
  const modalTitle = selectedAccountType
    ? getLocale('braveWalletCreateAccountImportAccount').replace('$1', selectedAccountType.name)
    : getLocale('braveWalletAddAccountImport')

  const filPrivateKeyFormatDescriptionTextParts =
      getLocaleWithTag('braveWalletFilImportPrivateKeyFormatDescription')

  // render
  return (
    <PopupModal title={modalTitle} onClose={onClickClose}>

      <DividerLine />

      {!selectedAccountType &&
        <SelectAccountType
          createAccountOptions={createAccountOptions}
          buttonText={getLocale('braveWalletAddAccountImport')}
          onSelectAccountType={onSelectAccountType}
        />
      }

      {selectedAccountType &&
        <StyledWrapper>
          <ImportDisclaimer>
            <DisclaimerText>{getLocale('braveWalletImportAccountDisclaimer')}</DisclaimerText>
          </ImportDisclaimer>

          {selectedAccountType?.coin === BraveWallet.CoinType.FIL &&
            <>
              <WarningWrapper>
                <WarningText>
                  {filPrivateKeyFormatDescriptionTextParts.beforeTag}
                    <a target='_blank' href={FILECOIN_FORMAT_DESCRIPTION_URL}>
                      {filPrivateKeyFormatDescriptionTextParts.duringTag}
                    </a>
                  {filPrivateKeyFormatDescriptionTextParts.afterTag}</WarningText>
              </WarningWrapper>

              <SelectWrapper>
                <Select value={filecoinNetwork} onChange={onChangeFilecoinNetwork}>
                  {FilecoinNetworkTypes.map((network, index) => {
                    const networkLocale = FilecoinNetworkLocaleMapping[network]
                    return (
                      <div data-value={network} key={index}>
                        {networkLocale}
                      </div>
                    )
                  })}
                </Select>
              </SelectWrapper>
            </>
          }

          {selectedAccountType?.coin === BraveWallet.CoinType.ETH &&
            <SelectWrapper>
              <Select
                value={importOption}
                onChange={setImportOption}
              >
                <div data-value='key'>
                  {getLocale('braveWalletImportAccountKey')}
                </div>
                <div data-value='file'>
                  {getLocale('braveWalletImportAccountFile')}
                </div>
              </Select>
            </SelectWrapper>
          }

          {hasImportError &&
            <ErrorText>{getLocale('braveWalletImportAccountError')}</ErrorText>
          }

          {importOption === 'key' && (
            <Input
              placeholder={getLocale('braveWalletImportAccountPlaceholder')}
              onChange={handlePrivateKeyChanged}
              type='password'
              autoFocus={true}
              autoComplete='off'
              onPaste={onClearClipboard}
            />
          )}

          {importOption !== 'key' && (
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
                onClick={onClickFileUpload}
              />
              <Input
                placeholder={`Origin ${getLocale('braveWalletCreatePasswordInput')}`}
                onChange={handlePasswordChanged}
                type='password'
                ref={passwordInputRef}
              />
            </>
          )}

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
            text={getLocale('braveWalletAddAccountImport')}
            buttonType='primary'
          />

        </StyledWrapper>
      }

    </PopupModal>
  )
}

export default ImportAccountModal
