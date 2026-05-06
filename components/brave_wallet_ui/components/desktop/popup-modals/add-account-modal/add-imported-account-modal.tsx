// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { assert, assertNotReached } from 'chrome://resources/js/assert.js'
import * as React from 'react'
import { useHistory, useParams } from 'react-router'
import { InputEventDetail } from '@brave/leo/react/input'
import Button from '@brave/leo/react/button'
import SegmentedControl from '@brave/leo/react/segmentedControl'
import SegmentedControlItem from '@brave/leo/react/segmentedControlItem'
import Alert from '@brave/leo/react/alert'
import { showAlert } from '@brave/leo/react/alertCenter'

// utils
import { FILECOIN_FORMAT_DESCRIPTION_URL } from '../../../../common/constants/urls'
import { getLocale, formatLocale } from '$web-common/locale'
import { copyToClipboard } from '../../../../utils/copy-to-clipboard'

// options
import { CreateAccountOptions } from '../../../../options/create-account-options'

// types
import {
  BraveWallet,
  CreateAccountOptionsType,
  WalletRoutes,
  DAppSupportedCoinTypes,
} from '../../../../constants/types'

// components
import { PopupModal } from '../index'
import { SelectAccountType } from './select-account-type/select-account-type'

// selectors
import { WalletSelectors } from '../../../../common/selectors'

// hooks
import { useSafeWalletSelector } from '../../../../common/hooks/use-safe-selector'
import {
  useGetVisibleNetworksQuery,
  useImportEthAccountFromJsonMutation,
  useImportEthAccountMutation,
  useImportBtcAccountMutation,
  useImportFilAccountMutation,
  useImportPolkadotAccountMutation,
  useImportSolAccountMutation,
} from '../../../../common/slices/api.slice'

// Styles
import {
  CreateAccountWrapper,
  CreateAccountContent,
  FileNameText,
  ErrorText,
  ImportButton,
  Input,
  NetworkIcon,
  NetworkName,
  NetworkDescription,
  JsonFileLabel,
} from './style'
import { Column, Row } from '../../../shared/style'
interface Params {
  accountTypeName: string
}

const reduceFileName = (address: string) => {
  const firstHalf = address.slice(0, 4)
  const secondHalf = address.slice(-4)
  const reduced = firstHalf.concat('......', secondHalf)
  return reduced
}

const filPrivateKeyFormatDescription = formatLocale(
  'braveWalletFilImportPrivateKeyFormatDescription',
  {
    $1: (content) => (
      <a
        target='_blank'
        href={FILECOIN_FORMAT_DESCRIPTION_URL}
        rel='noopener noreferrer'
      >
        {content}
      </a>
    ),
  },
)

export const ImportAccountModal = () => {
  // refs
  const passwordInputRef = React.useRef<HTMLInputElement>(null)
  const fileInputRef = React.useRef<HTMLInputElement>(null)

  // routing
  const history = useHistory()
  const { accountTypeName } = useParams<Params>()

  // redux
  const isBitcoinImportEnabled = useSafeWalletSelector(
    WalletSelectors.isBitcoinImportEnabled,
  )
  const isPolkadotEnabled = useSafeWalletSelector(
    WalletSelectors.isPolkadotEnabled,
  )

  // queries
  const { data: visibleNetworks = [] } = useGetVisibleNetworksQuery()

  // mutations
  const [importEthAccount] = useImportEthAccountMutation()
  const [importSolAccount] = useImportSolAccountMutation()
  const [importFilAccount] = useImportFilAccountMutation()
  const [importBtcAccount] = useImportBtcAccountMutation()
  const [importPolkadotAccount] = useImportPolkadotAccountMutation()
  const [importEthAccountFromJson] = useImportEthAccountFromJsonMutation()

  // memos
  const createAccountOptions = React.useMemo(() => {
    return CreateAccountOptions({
      visibleNetworks,
      isBitcoinEnabled: isBitcoinImportEnabled,
      isZCashEnabled: false, // No zcash imported accounts by now.
      isCardanoEnabled: false, // No cardano imported accounts by now.
      isPolkadotEnabled,
    })
  }, [visibleNetworks, isBitcoinImportEnabled, isPolkadotEnabled])

  const selectedAccountType = React.useMemo(() => {
    return createAccountOptions.find((option) => {
      return option.name.toLowerCase() === accountTypeName?.toLowerCase()
    })
  }, [accountTypeName, createAccountOptions])

  // state
  const [hasImportError, setHasImportError] = React.useState(false)
  const [fullLengthAccountName, setFullLengthAccountName] =
    React.useState<string>('')
  const accountName = fullLengthAccountName.substring(0, 30)
  const [importOption, setImportOption] = React.useState<string>('key')
  const [privateKey, setPrivateKey] = React.useState<string>('')
  const [file, setFile] = React.useState<HTMLInputElement['files']>()
  const [password, setPassword] = React.useState<string>('')

  // computed
  const isPolkadotImport =
    selectedAccountType?.coin === BraveWallet.CoinType.DOT
  const hasAccountNameError = accountName === ''
  const hasImportTypeError = isPolkadotImport
    ? !file || !password
    : importOption === 'key'
      ? !privateKey
      : !file
  const isDisabled = hasAccountNameError || hasImportTypeError
  const modalTitle = selectedAccountType
    ? getLocale('braveWalletCreateAccountImportAccount').replace(
        '$1',
        selectedAccountType.name,
      )
    : getLocale('braveWalletImportAccount')

  // methods
  const onClickClose = React.useCallback(() => {
    setHasImportError(false)
    history.push(WalletRoutes.Accounts)
  }, [history])

  const onClickBack = React.useCallback(() => {
    setImportOption('key')
    history.goBack()
  }, [history])

  const handleAccountNameChanged = React.useCallback(
    (detail: InputEventDetail) => {
      setFullLengthAccountName(detail.value)
      setHasImportError(false)
    },
    [],
  )

  const clearClipboard = React.useCallback(() => {
    copyToClipboard('')
  }, [])

  const handlePrivateKeyChanged = React.useCallback(
    (detail: InputEventDetail) => {
      clearClipboard()
      setPrivateKey(detail.value)
      setHasImportError(false)
    },
    [clearClipboard],
  )

  const onFileUpload = React.useCallback(
    (file: React.ChangeEvent<HTMLInputElement>) => {
      if (file.target.files) {
        setFile(file.target.files)
        setHasImportError(false)
        passwordInputRef.current?.focus()
      }
    },
    [],
  )

  const handlePasswordChanged = React.useCallback(
    (detail: InputEventDetail) => {
      setPassword(detail.value)
      setHasImportError(false)
      clearClipboard()
    },
    [clearClipboard],
  )

  const showSuccessAlert = React.useCallback(() => {
    showAlert({
      type: 'success',
      content: getLocale('braveWalletAccountImportedSuccessfully'),
      actions: [],
    })
  }, [])

  const onClickCreateAccount = React.useCallback(async () => {
    if (!selectedAccountType) {
      return
    }

    // Polkadot: JSON file + password (no key option)
    if (selectedAccountType.coin === BraveWallet.CoinType.DOT && file) {
      const index = file[0]
      const reader = new FileReader()
      reader.onload = async function () {
        if (reader.result && selectedAccountType.fixedNetwork) {
          try {
            await importPolkadotAccount({
              accountName,
              jsonExport: reader.result.toString().trim(),
              password,
              network: selectedAccountType.fixedNetwork,
            }).unwrap()
            showSuccessAlert()
            history.push(WalletRoutes.Accounts)
          } catch (error) {
            setHasImportError(true)
          }
        }
      }
      reader.readAsText(index)
      return
    }

    if (importOption === 'key') {
      const fixedNetwork = selectedAccountType.fixedNetwork
      try {
        if (selectedAccountType.coin === BraveWallet.CoinType.FIL) {
          assert(
            fixedNetwork === BraveWallet.FILECOIN_MAINNET
              || fixedNetwork === BraveWallet.FILECOIN_TESTNET,
          )
          await importFilAccount({
            accountName,
            privateKey,
            network: fixedNetwork,
          })
        } else if (selectedAccountType.coin === BraveWallet.CoinType.BTC) {
          assert(
            fixedNetwork === BraveWallet.BITCOIN_MAINNET
              || fixedNetwork === BraveWallet.BITCOIN_TESTNET,
          )
          await importBtcAccount({
            accountName,
            payload: privateKey,
            network: fixedNetwork,
          })
        } else if (selectedAccountType.coin === BraveWallet.CoinType.ETH) {
          await importEthAccount({
            accountName,
            privateKey,
          }).unwrap()
        } else if (selectedAccountType.coin === BraveWallet.CoinType.SOL) {
          await importSolAccount({
            accountName,
            privateKey,
          }).unwrap()
        } else {
          assertNotReached(`Unknown coin ${selectedAccountType.coin}`)
        }
      } catch (error) {
        setHasImportError(true)
        return
      }
      showSuccessAlert()
      history.push(WalletRoutes.Accounts)
      return
    }

    if (file) {
      const index = file[0]
      const reader = new FileReader()
      reader.onload = async function () {
        if (reader.result) {
          try {
            await importEthAccountFromJson({
              accountName,
              password,
              json: reader.result.toString().trim(),
            }).unwrap()
            showSuccessAlert()
            history.push(WalletRoutes.Accounts)
          } catch (error) {
            setHasImportError(true)
          }
        }
      }

      reader.readAsText(index)
    }
  }, [
    importOption,
    file,
    selectedAccountType,
    importFilAccount,
    importPolkadotAccount,
    accountName,
    privateKey,
    history,
    importBtcAccount,
    importEthAccount,
    importEthAccountFromJson,
    importSolAccount,
    password,
    showSuccessAlert,
  ])

  const handleKeyDown = React.useCallback(
    (detail: InputEventDetail) => {
      if (isDisabled) {
        return
      }
      if ((detail.innerEvent as unknown as KeyboardEvent).key === 'Enter') {
        onClickCreateAccount()
      }
    },
    [isDisabled, onClickCreateAccount],
  )

  const onSelectAccountType = React.useCallback(
    (accountType: CreateAccountOptionsType) => () => {
      history.push(
        WalletRoutes.ImportAccountModal.replace(
          ':accountTypeName?',
          accountType.name.toLowerCase(),
        ),
      )
    },
    [history],
  )

  const isDAppCoin =
    !!selectedAccountType
    && DAppSupportedCoinTypes.includes(selectedAccountType?.coin)

  // render
  return (
    <PopupModal
      title={modalTitle}
      onClose={onClickClose}
      onBack={selectedAccountType ? onClickBack : undefined}
      headerPaddingHorizontal='32px'
      headerPaddingVertical='32px'
      headerPaddingMobile='20px'
    >
      {!selectedAccountType && (
        <SelectAccountType
          createAccountOptions={createAccountOptions}
          onSelectAccountType={onSelectAccountType}
        />
      )}

      {selectedAccountType && (
        <CreateAccountWrapper width='100%'>
          <Column gap='16px'>
            {isDAppCoin && (
              <Alert type='warning'>
                {getLocale('braveWalletImportAccountDisclaimer')}
              </Alert>
            )}

            {selectedAccountType.coin === BraveWallet.CoinType.FIL && (
              <Alert type='warning'>{filPrivateKeyFormatDescription}</Alert>
            )}
            {selectedAccountType.coin === BraveWallet.CoinType.BTC && (
              <Alert type='warning'>
                {getLocale(
                  'braveWalletBtcImportPrivateKeyFormatDescription',
                ).replace(
                  '$1',
                  selectedAccountType.fixedNetwork
                    === BraveWallet.BITCOIN_MAINNET
                    ? 'zprv'
                    : 'tprv',
                )}
              </Alert>
            )}
            {selectedAccountType.coin === BraveWallet.CoinType.DOT && (
              <Alert type='warning'>
                {getLocale('braveWalletPolkadotImportJsonDescription')}
              </Alert>
            )}
          </Column>
          <Column gap='16px'>
            <NetworkIcon src={selectedAccountType.icon} />
            <CreateAccountContent>
              <NetworkName textColor='primary'>
                {selectedAccountType.name}
              </NetworkName>
              <NetworkDescription textColor='tertiary'>
                {selectedAccountType.description}
              </NetworkDescription>
            </CreateAccountContent>
          </Column>
          <Column
            gap='16px'
            width='100%'
          >
            {selectedAccountType.coin === BraveWallet.CoinType.ETH && (
              <>
                <SegmentedControl
                  value={importOption}
                  onChange={({ value }) => {
                    if (value) {
                      setImportOption(value)
                    }
                  }}
                  size='small'
                >
                  <SegmentedControlItem value='key'>
                    {getLocale('braveWalletImportAccountKey')}
                  </SegmentedControlItem>
                  <SegmentedControlItem value='file'>
                    {getLocale('braveWalletImportAccountFile')}
                  </SegmentedControlItem>
                </SegmentedControl>
                <NetworkDescription textColor='tertiary'>
                  {importOption === 'key'
                    ? getLocale('braveWalletImportAccountPrivateKeyDescription')
                    : getLocale('braveWalletImportAccountJsonFileDescription')}
                </NetworkDescription>
              </>
            )}

            {hasImportError && (
              <ErrorText>
                {getLocale('braveWalletImportAccountError')}
              </ErrorText>
            )}

            {!isPolkadotImport && importOption === 'key' ? (
              <Input
                placeholder={getLocale('braveWalletImportAccountKey')}
                onBlur={clearClipboard}
                type='password'
                onInput={handlePrivateKeyChanged}
                onKeyDown={handleKeyDown}
              >
                {getLocale('braveWalletImportAccountPlaceholder')}
              </Input>
            ) : (
              <>
                <Column
                  gap='8px'
                  alignItems='flex-start'
                  justifyContent='flex-start'
                  width='100%'
                >
                  <JsonFileLabel textColor='primary'>
                    {getLocale('braveWalletUploadJsonFile')}
                  </JsonFileLabel>
                  <Row
                    justifyContent='flex-start'
                    gap='8px'
                  >
                    <ImportButton
                      kind='outline'
                      size='small'
                      onClick={() => fileInputRef.current?.click()}
                    >
                      {getLocale('braveWalletImportAccountUploadButton')}
                    </ImportButton>
                    <FileNameText textColor='tertiary'>
                      {file
                        ? reduceFileName(file[0].name)
                        : getLocale(
                            'braveWalletImportAccountUploadPlaceholder',
                          )}
                    </FileNameText>
                  </Row>
                  <input
                    ref={fileInputRef}
                    type='file'
                    id='recoverFile'
                    name='recoverFile'
                    style={{ display: 'none' }}
                    onChange={onFileUpload}
                  />
                </Column>
                <Input
                  placeholder={getLocale('braveWalletInputLabelPassword')}
                  onInput={handlePasswordChanged}
                  onKeyDown={handleKeyDown}
                  onBlur={clearClipboard}
                  type='password'
                  ref={passwordInputRef}
                >
                  {getLocale('braveWalletEnterPasswordIfApplicable')}
                </Input>
              </>
            )}
            <Input
              value={accountName}
              placeholder={getLocale('braveWalletAccountName')}
              onInput={handleAccountNameChanged}
              onKeyDown={handleKeyDown}
              showErrors={hasAccountNameError}
              maxlength={BraveWallet.ACCOUNT_NAME_MAX_CHARACTER_LENGTH}
            >
              {getLocale('braveWalletAddAccountPlaceholder')}
            </Input>
          </Column>
          <Row gap='16px'>
            <Button
              onClick={onClickClose}
              kind='outline'
            >
              {getLocale('braveWalletButtonCancel')}
            </Button>
            <Button
              onClick={onClickCreateAccount}
              isDisabled={isDisabled}
              kind='filled'
            >
              {getLocale('braveWalletImportAccount')}
            </Button>
          </Row>
        </CreateAccountWrapper>
      )}
    </PopupModal>
  )
}

export default ImportAccountModal
