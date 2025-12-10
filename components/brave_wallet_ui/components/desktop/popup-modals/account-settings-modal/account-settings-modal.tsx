// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query'
import ProgressRingReact from '@brave/leo/react/progressRing'
import Input, { InputEventDetail } from '@brave/leo/react/input'
import SegmentedControlItem from '@brave/leo/react/segmentedControlItem'

// redux
import { useDispatch, useSelector } from 'react-redux'

// actions
import {
  AccountsTabState,
  AccountsTabActions,
} from '../../../../page/reducers/accounts-tab-reducer'

// selectors
import { useSafeWalletSelector } from '../../../../common/hooks/use-safe-selector'
import { WalletSelectors } from '../../../../common/selectors'

// utils
import { getLocale, formatLocale } from '$web-common/locale'
import getAPIProxy from '../../../../common/async/bridge'

// constants
import { FILECOIN_FORMAT_DESCRIPTION_URL } from '../../../../common/constants/urls'

// options
import { AccountButtonOptions } from '../../../../options/account-list-button-options'

// types
import {
  BraveWallet,
  zcashAddressOptionType,
} from '../../../../constants/types'

// components
import { CopyTooltip } from '../../../shared/copy-tooltip/copy-tooltip'
import PopupModal from '../index'
import PasswordInput from '../../../shared/password-input/index'
import {
  CreateAccountIcon, //
} from '../../../shared/create-account-icon/create-account-icon'

// hooks
import { useIsMounted } from '../../../../common/hooks/useIsMounted'
import { usePasswordAttempts } from '../../../../common/hooks/use-password-attempts'
import {
  useGetQrCodeImageQuery,
  useGetZCashAccountInfoQuery,
  useUpdateAccountNameMutation, //
} from '../../../../common/slices/api.slice'
import {
  useReceiveAddressQuery, //
} from '../../../../common/slices/api.slice.extra'

// style
import {
  StyledWrapper,
  QRCodeWrapper,
  AddressButton,
  ButtonRow,
  CopyIcon,
  PrivateKeyWrapper,
  PrivateKeyBubble,
  ButtonWrapper,
  ErrorText,
  Line,
  QRCodeImage,
  EditWrapper,
  Alert,
  ControlsWrapper,
  SegmentedControl,
} from './account-settings-modal.style'
import {
  Column,
  LeoSquaredButton,
  Text,
  VerticalSpacer,
} from '../../../shared/style'
import { Skeleton } from '../../../shared/loading-skeleton/styles'

const zcashAddressOptions: zcashAddressOptionType[] = [
  {
    addressType: 'unified',
    label: 'braveWalletUnified',
  },
  {
    addressType: 'shielded',
    label: 'braveWalletShielded',
  },
  {
    addressType: 'transparent',
    label: 'braveWalletTransparent',
  },
]
interface DepositModalProps {
  selectedAccount: BraveWallet.AccountInfo
}

const filPrivateKeyFormatDescription = formatLocale(
  'braveWalletFilExportPrivateKeyFormatDescription',
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

export const DepositModal = ({ selectedAccount }: DepositModalProps) => {
  // state
  const [selectedZCashAddressOption, setSelectedZCashAddressOption] =
    React.useState<string>('unified')

  // redux
  const isZCashShieldedTransactionsEnabled = useSafeWalletSelector(
    WalletSelectors.isZCashShieldedTransactionsEnabled,
  )

  // queries and memos
  const { receiveAddress } = useReceiveAddressQuery(selectedAccount.accountId)
  const { data: zcashAccountInfo } = useGetZCashAccountInfoQuery(
    isZCashShieldedTransactionsEnabled
      && selectedAccount.accountId.coin === BraveWallet.CoinType.ZEC
      ? selectedAccount.accountId
      : skipToken,
  )

  const displayAddress = React.useMemo(() => {
    if (
      isZCashShieldedTransactionsEnabled
      && selectedAccount.accountId.coin === BraveWallet.CoinType.ZEC
      && zcashAccountInfo?.accountShieldBirthday
    ) {
      return selectedZCashAddressOption === 'unified'
        ? zcashAccountInfo.unifiedAddress
        : selectedZCashAddressOption === 'shielded'
          ? zcashAccountInfo.orchardAddress
          : zcashAccountInfo.nextTransparentReceiveAddress.addressString
    }
    return receiveAddress
  }, [
    isZCashShieldedTransactionsEnabled,
    selectedAccount,
    receiveAddress,
    zcashAccountInfo,
    selectedZCashAddressOption,
  ])

  const { data: qrCode, isFetching: isLoadingQrCode } = useGetQrCodeImageQuery(
    displayAddress || skipToken,
  )

  // render
  return (
    <Column padding='0px 16px'>
      <Column
        gap='8px'
        margin='0px 0px 24px 0px'
      >
        <CreateAccountIcon
          account={selectedAccount}
          size='huge'
        />
        <Text
          textSize='14px'
          textColor='primary'
        >
          {selectedAccount.name}
        </Text>
      </Column>

      {zcashAccountInfo && zcashAccountInfo.accountShieldBirthday && (
        <ControlsWrapper width='unset'>
          <SegmentedControl
            value={selectedZCashAddressOption}
            onChange={({ value }) => {
              if (value) {
                setSelectedZCashAddressOption(value)
              }
            }}
          >
            {zcashAddressOptions.map((option) => (
              <SegmentedControlItem
                key={option.addressType}
                value={option.addressType}
              >
                {getLocale(option.label)}
              </SegmentedControlItem>
            ))}
          </SegmentedControl>
        </ControlsWrapper>
      )}

      <QRCodeWrapper>
        {isLoadingQrCode || !displayAddress ? (
          <ProgressRingReact mode='indeterminate' />
        ) : (
          <QRCodeImage src={qrCode} />
        )}
      </QRCodeWrapper>

      {displayAddress ? (
        <CopyTooltip text={displayAddress}>
          <AddressButton>
            {displayAddress}
            <CopyIcon />
          </AddressButton>
        </CopyTooltip>
      ) : (
        <Skeleton
          height={'20px'}
          width={'300px'}
        />
      )}
      <VerticalSpacer space={20} />
    </Column>
  )
}

export const AccountSettingsModal = () => {
  // custom hooks
  const isMounted = useIsMounted()

  // redux
  const dispatch = useDispatch()

  // accounts tab state
  const selectedAccount = useSelector(
    ({ accountsTab }: { accountsTab: AccountsTabState }) =>
      accountsTab.selectedAccount,
  )
  const accountModalType = useSelector(
    ({ accountsTab }: { accountsTab: AccountsTabState }) =>
      accountsTab.accountModalType,
  )

  // state
  const [fullLengthAccountName, setFullLengthAccountName] =
    React.useState<string>(selectedAccount?.name ?? '')
  const accountName = fullLengthAccountName.substring(0, 30)
  const [updateError, setUpdateError] = React.useState<boolean>(false)
  const [password, setPassword] = React.useState<string>('')
  const [privateKey, setPrivateKey] = React.useState<string>('')
  const [isCorrectPassword, setIsCorrectPassword] =
    React.useState<boolean>(true)
  const [showEncryptionPassword, setShowEncryptionPassword] =
    React.useState<boolean>(false)
  const [encryptionPassword, setEncryptionPassword] = React.useState<string>('')
  const [encryptionPasswordConfirm, setEncryptionPasswordConfirm] =
    React.useState<string>('')
  const [encryptionPasswordError, setEncryptionPasswordError] =
    React.useState<string>('')

  // mutations
  const [updateAccountName] = useUpdateAccountNameMutation()

  // custom hooks
  const { attemptPasswordEntry } = usePasswordAttempts()

  // Helper function to download JSON file
  const downloadJsonFile = React.useCallback(
    (jsonContent: string, filename: string) => {
      const blob = new Blob([jsonContent], { type: 'application/json' })
      const url = URL.createObjectURL(blob)
      const link = document.createElement('a')
      link.href = url
      link.download = filename
      document.body.appendChild(link)
      link.click()
      document.body.removeChild(link)
      URL.revokeObjectURL(url)
    },
    [],
  )

  // Validate encryption password strength (minimum 8 characters)
  const validateEncryptionPassword = React.useCallback(
    (password: string): boolean => {
      return password.length >= 8
    },
    [],
  )

  // Handler for confirming encryption password and showing key
  const onConfirmEncryptionPassword = React.useCallback(async () => {
    if (
      !encryptionPassword
      || !encryptionPasswordConfirm
      || encryptionPassword !== encryptionPasswordConfirm
      || !validateEncryptionPassword(encryptionPassword)
      || !selectedAccount
    ) {
      return
    }

    // Get the encrypted key with the provided password
    const { privateKey: encryptedKey } =
      await getAPIProxy().keyringService.encodePolkadotKeyForExport(
        selectedAccount.accountId,
        password,
        encryptionPassword,
      )

    if (encryptedKey && isMounted) {
      // Show the key
      setPrivateKey(encryptedKey)
      // Reset encryption password state
      setShowEncryptionPassword(false)
      setEncryptionPassword('')
      setPassword('')
      setEncryptionPasswordConfirm('')
      setEncryptionPasswordError('')
    }
  }, [
    encryptionPassword,
    encryptionPasswordConfirm,
    validateEncryptionPassword,
    selectedAccount,
    password,
    isMounted,
  ])

  // Handler for downloading the already-encrypted key
  const onDownloadPolkadotKey = React.useCallback(() => {
    if (!privateKey || !selectedAccount) {
      return
    }
    const accountName = selectedAccount.name || 'account'
    const sanitizedAccountName = accountName.replace(/[^a-z0-9]/gi, '_')
    const filename = `${sanitizedAccountName}_export.json`
    downloadJsonFile(privateKey, filename)
  }, [privateKey, selectedAccount, downloadJsonFile])

  const onCancelEncryptionPassword = React.useCallback(() => {
    setShowEncryptionPassword(false)
    setEncryptionPassword('')
    setEncryptionPasswordConfirm('')
    setEncryptionPasswordError('')
    // Also clear the wallet password state
    setPassword('')
    setIsCorrectPassword(true)
  }, [])

  // methods
  const onViewPrivateKey = React.useCallback(
    async (accountId: BraveWallet.AccountId) => {
      const { privateKey } =
        await getAPIProxy().keyringService.encodePrivateKeyForExport(
          accountId,
          password,
        )
      if (isMounted) {
        return setPrivateKey(privateKey || '')
      }
    },
    [password, isMounted],
  )

  const onDoneViewingPrivateKey = React.useCallback(() => {
    setPrivateKey('')
  }, [])

  const handleAccountNameChanged = (detail: InputEventDetail) => {
    setFullLengthAccountName(detail.value)
    setUpdateError(false)
  }

  const onClose = React.useCallback(() => {
    dispatch(AccountsTabActions.setShowAccountModal(false))
    dispatch(AccountsTabActions.setAccountModalType('deposit'))
  }, [dispatch])

  const onSubmitUpdateName = React.useCallback(async () => {
    if (!selectedAccount || !accountName) {
      return
    }

    try {
      await updateAccountName({
        accountId: selectedAccount.accountId,
        name: accountName,
      }).unwrap()
      onClose()
    } catch (error) {
      setUpdateError(true)
    }
  }, [selectedAccount, accountName, updateAccountName, onClose])

  const onShowPrivateKey = async () => {
    if (!password || !selectedAccount) {
      // require password to view key
      return
    }

    // entered password must be correct
    const isPasswordValid = await attemptPasswordEntry(password)

    if (!isPasswordValid) {
      setIsCorrectPassword(isPasswordValid) // set or clear error
      return // need valid password to continue
    }

    // For Polkadot accounts, ask for encryption password first
    if (selectedAccount.accountId.coin === BraveWallet.CoinType.DOT) {
      // Clear any existing private key first
      setPrivateKey('')
      setIsCorrectPassword(true)
      // Show encryption password input
      setShowEncryptionPassword(true)
      setEncryptionPassword('')
      setEncryptionPasswordConfirm('')
      setEncryptionPasswordError('')
      return
    }

    // clear entered password & error
    setPassword('')
    setIsCorrectPassword(true)

    onViewPrivateKey(selectedAccount.accountId)
  }

  const onHidePrivateKey = () => {
    onDoneViewingPrivateKey()
    setPrivateKey('')
    setShowEncryptionPassword(false)
    setEncryptionPassword('')
    setEncryptionPasswordConfirm('')
    setEncryptionPasswordError('')
  }

  const onClickClose = () => {
    onHidePrivateKey()
    setUpdateError(false)
    onClose()
  }

  const handleKeyDown = (detail: InputEventDetail) => {
    if ((detail.innerEvent as unknown as KeyboardEvent).key === 'Enter') {
      onSubmitUpdateName()
    }
  }

  const onPasswordChange = (value: string): void => {
    setIsCorrectPassword(true) // clear error
    setPassword(value)
  }

  const onEncryptionPasswordChange = React.useCallback(
    (value: string) => {
      setEncryptionPassword(value)
      if (value && !validateEncryptionPassword(value)) {
        setEncryptionPasswordError(
          getLocale('braveWalletAccountSettingsEncryptionPasswordTooShort'),
        )
      } else {
        setEncryptionPasswordError('')
      }
    },
    [validateEncryptionPassword],
  )

  const onEncryptionPasswordConfirmChange = React.useCallback(
    (value: string) => {
      setEncryptionPasswordConfirm(value)
    },
    [],
  )

  const handlePasswordKeyDown = (
    event: React.KeyboardEvent<HTMLInputElement>,
  ) => {
    if (event.key === 'Enter') {
      onShowPrivateKey()
    }
  }

  // memos
  const modalTitle = React.useMemo((): string => {
    if (accountModalType) {
      return (
        AccountButtonOptions.find((option) => option.id === accountModalType)
          ?.name ?? ''
      )
    }
    return ''
  }, [accountModalType])

  // computed
  const showNameInputErrors = accountName === ''

  // render
  return (
    <PopupModal
      title={getLocale(modalTitle)}
      onClose={onClickClose}
    >
      <Line />
      <StyledWrapper>
        {selectedAccount && accountModalType === 'deposit' && (
          <DepositModal selectedAccount={selectedAccount} />
        )}
        {accountModalType === 'edit' && (
          <EditWrapper>
            <Input
              value={accountName}
              placeholder={getLocale('braveWalletAddAccountPlaceholder')}
              onInput={handleAccountNameChanged}
              onKeyDown={handleKeyDown}
              showErrors={showNameInputErrors}
              size='large'
              maxlength={BraveWallet.ACCOUNT_NAME_MAX_CHARACTER_LENGTH}
            >
              {
                // Label
                getLocale('braveWalletAddAccountPlaceholder')
              }
            </Input>

            {updateError && (
              <ErrorText>
                {getLocale('braveWalletAccountSettingsUpdateError')}
              </ErrorText>
            )}

            <ButtonRow>
              <LeoSquaredButton
                onClick={onSubmitUpdateName}
                isDisabled={showNameInputErrors}
                kind='filled'
              >
                {getLocale('braveWalletAccountSettingsSave')}
              </LeoSquaredButton>
            </ButtonRow>
          </EditWrapper>
        )}
        {accountModalType === 'privateKey' && (
          <PrivateKeyWrapper>
            <Alert type='warning'>
              {getLocale('braveWalletAccountSettingsDisclaimer')}
            </Alert>
            {selectedAccount
            && selectedAccount.accountId.coin === BraveWallet.CoinType.DOT
            && showEncryptionPassword
            && !privateKey ? (
              // Show encryption password input for Polkadot
              <>
                <Alert type='info'>
                  {getLocale(
                    'braveWalletAccountSettingsEnterPasswordToEncrypt',
                  )}
                </Alert>
                <PasswordInput
                  placeholder={getLocale(
                    'braveWalletAccountSettingsEncryptionPassword',
                  )}
                  onChange={onEncryptionPasswordChange}
                  hasError={!!encryptionPasswordError}
                  error={encryptionPasswordError}
                  autoFocus={true}
                  value={encryptionPassword}
                />
                <VerticalSpacer space={16} />
                <PasswordInput
                  placeholder={getLocale(
                    'braveWalletAccountSettingsConfirmEncryptionPassword',
                  )}
                  onChange={onEncryptionPasswordConfirmChange}
                  hasError={
                    encryptionPasswordConfirm !== ''
                    && encryptionPassword !== encryptionPasswordConfirm
                  }
                  error={
                    encryptionPasswordConfirm !== ''
                    && encryptionPassword !== encryptionPasswordConfirm
                      ? getLocale(
                          'braveWalletAccountSettingsPasswordsDoNotMatch',
                        )
                      : ''
                  }
                  autoFocus={false}
                  value={encryptionPasswordConfirm}
                  onKeyDown={(event: React.KeyboardEvent<HTMLInputElement>) => {
                    if (
                      event.key === 'Enter'
                      && encryptionPassword
                      && encryptionPasswordConfirm
                      && encryptionPassword === encryptionPasswordConfirm
                      && validateEncryptionPassword(encryptionPassword)
                    ) {
                      onConfirmEncryptionPassword()
                    }
                  }}
                />
              </>
            ) : privateKey ? (
              // Show the key
              <>
                {selectedAccount?.accountId.coin
                  === BraveWallet.CoinType.FIL && (
                  <Alert type='warning'>{filPrivateKeyFormatDescription}</Alert>
                )}
                <CopyTooltip
                  text={privateKey}
                  isConfidential={true}
                >
                  <PrivateKeyBubble>{privateKey}</PrivateKeyBubble>
                </CopyTooltip>
              </>
            ) : (
              // Show wallet password input
              <PasswordInput
                placeholder={getLocale(
                  'braveWalletEnterYourBraveWalletPassword',
                )}
                onChange={onPasswordChange}
                hasError={!!password && !isCorrectPassword}
                error={getLocale('braveWalletLockScreenError')}
                autoFocus={false}
                value={password}
                onKeyDown={handlePasswordKeyDown}
              />
            )}
            <ButtonWrapper>
              {selectedAccount
              && selectedAccount.accountId.coin === BraveWallet.CoinType.DOT
              && showEncryptionPassword
              && !privateKey ? (
                // Show Cancel and Confirm buttons for encryption password
                <ButtonRow>
                  <LeoSquaredButton
                    onClick={onCancelEncryptionPassword}
                    kind='outline'
                  >
                    {getLocale('braveWalletButtonCancel')}
                  </LeoSquaredButton>
                  <LeoSquaredButton
                    onClick={onConfirmEncryptionPassword}
                    kind='filled'
                    isDisabled={
                      !encryptionPassword
                      || !encryptionPasswordConfirm
                      || encryptionPassword !== encryptionPasswordConfirm
                      || !validateEncryptionPassword(encryptionPassword)
                      || !!encryptionPasswordError
                    }
                  >
                    {getLocale('braveWalletAccountSettingsShowKey')}
                  </LeoSquaredButton>
                </ButtonRow>
              ) : privateKey ? (
                // Show Download and Hide buttons when key is visible
                <ButtonRow>
                  {selectedAccount?.accountId.coin
                    === BraveWallet.CoinType.DOT && (
                    <LeoSquaredButton
                      onClick={onDownloadPolkadotKey}
                      kind='outline'
                    >
                      {getLocale('braveWalletAccountSettingsDownloadKey')}
                    </LeoSquaredButton>
                  )}
                  <LeoSquaredButton
                    onClick={onHidePrivateKey}
                    kind='filled'
                  >
                    {getLocale('braveWalletAccountSettingsHideKey')}
                  </LeoSquaredButton>
                </ButtonRow>
              ) : (
                // Show Show Key button when no key is visible
                <LeoSquaredButton
                  onClick={onShowPrivateKey}
                  kind='filled'
                  isDisabled={password ? !isCorrectPassword : true}
                >
                  {getLocale('braveWalletAccountSettingsShowKey')}
                </LeoSquaredButton>
              )}
            </ButtonWrapper>
          </PrivateKeyWrapper>
        )}
      </StyledWrapper>
    </PopupModal>
  )
}

export default AccountSettingsModal
