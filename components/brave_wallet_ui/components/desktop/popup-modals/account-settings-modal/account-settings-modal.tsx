// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { skipToken } from '@reduxjs/toolkit/query'
import ProgressRingReact from '@brave/leo/react/progressRing'

// redux
import { useDispatch, useSelector } from 'react-redux'

// actions
import {
  AccountsTabState,
  AccountsTabActions
} from '../../../../page/reducers/accounts-tab-reducer'

// utils
import { getLocale, getLocaleWithTag } from '../../../../../common/locale'

// constants
import { FILECOIN_FORMAT_DESCRIPTION_URL } from '../../../../common/constants/urls'

// options
import { AccountButtonOptions } from '../../../../options/account-list-button-options'

// types
import { BraveWallet } from '../../../../constants/types'

// components
import { NavButton } from '../../../extension/buttons/nav-button/index'
import { CopyTooltip } from '../../../shared/copy-tooltip/copy-tooltip'
import PopupModal from '../index'
import PasswordInput from '../../../shared/password-input/index'

// hooks
import { useIsMounted } from '../../../../common/hooks/useIsMounted'
import { usePasswordAttempts } from '../../../../common/hooks/use-password-attempts'
import { useApiProxy } from '../../../../common/hooks/use-api-proxy'
import { useAccountOrb } from '../../../../common/hooks/use-orb'
import {
  useGetQrCodeImageQuery,
  useUpdateAccountNameMutation //
} from '../../../../common/slices/api.slice'
import {
  useReceiveAddressQuery //
} from '../../../../common/slices/api.slice.extra'

// style
import {
  Input,
  StyledWrapper,
  QRCodeWrapper,
  AddressButton,
  ButtonRow,
  CopyIcon,
  PrivateKeyWrapper,
  WarningText,
  WarningWrapper,
  PrivateKeyBubble,
  ButtonWrapper,
  ErrorText,
  Line,
  NameAndIcon,
  AccountCircle,
  AccountName,
  QRCodeImage
} from './account-settings-modal.style'
import { VerticalSpacer } from '../../../shared/style'
import { Skeleton } from '../../../shared/loading-skeleton/styles'

interface DepositModalProps {
  selectedAccount: BraveWallet.AccountInfo
}

export const DepositModal = ({ selectedAccount }: DepositModalProps) => {
  const orb = useAccountOrb(selectedAccount)

  // queries
  const { receiveAddress } = useReceiveAddressQuery(selectedAccount.accountId)
  const { data: qrCode, isFetching: isLoadingQrCode } = useGetQrCodeImageQuery(
    receiveAddress || skipToken
  )

  // render
  return (
    <>
      <NameAndIcon>
        <AccountCircle orb={orb} />
        <AccountName>{selectedAccount.name}</AccountName>
      </NameAndIcon>

      <QRCodeWrapper>
        {isLoadingQrCode || !receiveAddress ? (
          <ProgressRingReact mode='indeterminate' />
        ) : (
          <QRCodeImage src={qrCode} />
        )}
      </QRCodeWrapper>

      {receiveAddress ? (
        <CopyTooltip text={receiveAddress}>
          <AddressButton>
            {receiveAddress}
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
    </>
  )
}

export const AccountSettingsModal = () => {
  // custom hooks
  const isMounted = useIsMounted()
  const { keyringService } = useApiProxy()
  // redux
  const dispatch = useDispatch()

  // accounts tab state
  const selectedAccount = useSelector(
    ({ accountsTab }: { accountsTab: AccountsTabState }) =>
      accountsTab.selectedAccount
  )
  const accountModalType = useSelector(
    ({ accountsTab }: { accountsTab: AccountsTabState }) =>
      accountsTab.accountModalType
  )

  // state
  const [accountName, setAccountName] = React.useState<string>(
    selectedAccount?.name ?? ''
  )
  const [updateError, setUpdateError] = React.useState<boolean>(false)
  const [password, setPassword] = React.useState<string>('')
  const [privateKey, setPrivateKey] = React.useState<string>('')
  const [isCorrectPassword, setIsCorrectPassword] =
    React.useState<boolean>(true)

  // mutations
  const [updateAccountName] = useUpdateAccountNameMutation()

  // custom hooks
  const { attemptPasswordEntry } = usePasswordAttempts()

  // methods
  const onViewPrivateKey = React.useCallback(
    async (accountId: BraveWallet.AccountId) => {
      const { privateKey } = await keyringService.encodePrivateKeyForExport(
        accountId,
        password
      )
      if (isMounted) {
        return setPrivateKey(privateKey)
      }
    },
    [password, keyringService, isMounted]
  )

  const onDoneViewingPrivateKey = React.useCallback(() => {
    setPrivateKey('')
  }, [])

  const handleAccountNameChanged = (
    event: React.ChangeEvent<HTMLInputElement>
  ) => {
    setAccountName(event.target.value)
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
        name: accountName
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

    // clear entered password & error
    setPassword('')
    setIsCorrectPassword(true)

    onViewPrivateKey(selectedAccount.accountId)
  }

  const onHidePrivateKey = () => {
    onDoneViewingPrivateKey()
    setPrivateKey('')
  }

  const onClickClose = () => {
    onHidePrivateKey()
    setUpdateError(false)
    onClose()
  }

  const handleKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter' && accountName) {
      onSubmitUpdateName()
    }
  }

  const onPasswordChange = (value: string): void => {
    setIsCorrectPassword(true) // clear error
    setPassword(value)
  }

  const handlePasswordKeyDown = (
    event: React.KeyboardEvent<HTMLInputElement>
  ) => {
    if (event.key === 'Enter') {
      onShowPrivateKey()
    }
  }

  const filPrivateKeyFormatDescriptionTextParts = getLocaleWithTag(
    'braveWalletFilExportPrivateKeyFormatDescription'
  )

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
          <>
            <Input
              value={accountName}
              placeholder={getLocale('braveWalletAddAccountPlaceholder')}
              onChange={handleAccountNameChanged}
              onKeyDown={handleKeyDown}
            />
            {updateError && (
              <ErrorText>
                {getLocale('braveWalletAccountSettingsUpdateError')}
              </ErrorText>
            )}
            <ButtonRow>
              <NavButton
                onSubmit={onSubmitUpdateName}
                disabled={!accountName}
                text={getLocale('braveWalletAccountSettingsSave')}
                buttonType='secondary'
              />
            </ButtonRow>
          </>
        )}
        {accountModalType === 'privateKey' && (
          <PrivateKeyWrapper>
            <WarningWrapper>
              <WarningText>
                {getLocale('braveWalletAccountSettingsDisclaimer')}
              </WarningText>
            </WarningWrapper>
            {privateKey ? (
              <>
                {selectedAccount?.accountId.coin ===
                  BraveWallet.CoinType.FIL && (
                  <WarningWrapper>
                    <WarningText>
                      {filPrivateKeyFormatDescriptionTextParts.beforeTag}
                      <a
                        target='_blank'
                        href={FILECOIN_FORMAT_DESCRIPTION_URL}
                        rel='noopener noreferrer'
                      >
                        {filPrivateKeyFormatDescriptionTextParts.duringTag}
                      </a>
                      {filPrivateKeyFormatDescriptionTextParts.afterTag}
                    </WarningText>
                  </WarningWrapper>
                )}
                <CopyTooltip text={privateKey}>
                  <PrivateKeyBubble>{privateKey}</PrivateKeyBubble>
                </CopyTooltip>
              </>
            ) : (
              <PasswordInput
                placeholder={getLocale(
                  'braveWalletEnterYourBraveWalletPassword'
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
              <NavButton
                onSubmit={!privateKey ? onShowPrivateKey : onHidePrivateKey}
                text={getLocale(
                  !privateKey
                    ? 'braveWalletAccountSettingsShowKey'
                    : 'braveWalletAccountSettingsHideKey'
                )}
                buttonType='primary'
                disabled={
                  privateKey ? false : password ? !isCorrectPassword : true
                }
              />
            </ButtonWrapper>
          </PrivateKeyWrapper>
        )}
      </StyledWrapper>
    </PopupModal>
  )
}

export default AccountSettingsModal
