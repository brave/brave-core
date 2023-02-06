// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// redux
import {
  useDispatch,
  useSelector
} from 'react-redux'

// // actions
import { WalletPageActions } from '../../../../page/actions'
import {
  AccountsTabState,
  AccountsTabActions
} from '../../../../page/reducers/accounts-tab-reducer'

// utils
import { getLocale, getLocaleWithTag } from '../../../../../common/locale'
import { generateQRCode } from '../../../../utils/qr-code-utils'

// constants
import { FILECOIN_FORMAT_DESCRIPTION_URL } from '../../../../common/constants/urls'

// options
import { AccountButtonOptions } from '../../../../options/account-list-button-options'

// types
import {
  BraveWallet
} from '../../../../constants/types'

// components
import { NavButton } from '../../../extension'
import { CopyTooltip } from '../../../shared/copy-tooltip/copy-tooltip'
import PopupModal from '../index'
import PasswordInput from '../../../shared/password-input/index'
import { create } from 'ethereum-blockies'

// hooks
import { useIsMounted } from '../../../../common/hooks/useIsMounted'
import { usePasswordAttempts } from '../../../../common/hooks/use-password-attempts'
import { useApiProxy } from '../../../../common/hooks/use-api-proxy'

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
  AccountName
} from './account-settings-modal.style'
import { VerticalSpacer } from '../../../shared/style'

export const AccountSettingsModal = () => {
  // custom hooks
  const isMounted = useIsMounted()
  const { keyringService } = useApiProxy()
  // redux
  const dispatch = useDispatch()

  // accounts tab state
  const selectedAccount = useSelector(({ accountsTab }: { accountsTab: AccountsTabState }) => accountsTab.selectedAccount)
  const accountModalType = useSelector(({ accountsTab }: { accountsTab: AccountsTabState }) => accountsTab.accountModalType)

  // state
  const [accountName, setAccountName] = React.useState<string>(selectedAccount?.name ?? '')
  const [updateError, setUpdateError] = React.useState<boolean>(false)
  const [password, setPassword] = React.useState<string>('')
  const [privateKey, setPrivateKey] = React.useState<string>('')
  const [isCorrectPassword, setIsCorrectPassword] = React.useState<boolean>(true)
  const [qrCode, setQRCode] = React.useState<string>('')

  // custom hooks
  const { attemptPasswordEntry } = usePasswordAttempts()

  // methods
  const onViewPrivateKey = React.useCallback(async (
    address: string,
    coin: BraveWallet.CoinType
  ) => {
    const { privateKey } = await keyringService.encodePrivateKeyForExport(
      address,
      password,
      coin
    )
    if (isMounted) {
      return setPrivateKey(privateKey)
    }
  }, [password, keyringService, isMounted])

  const onDoneViewingPrivateKey = React.useCallback(() => {
    setPrivateKey('')
  }, [])

  const handleAccountNameChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    setAccountName(event.target.value)
    setUpdateError(false)
  }

  const onClose = () => {
    dispatch(AccountsTabActions.setShowAccountModal(false))
    dispatch(AccountsTabActions.setAccountModalType('deposit'))
  }

  const onSubmitUpdateName = React.useCallback(() => {
    if (selectedAccount) {
      const isDerived = selectedAccount.accountType === 'Primary'
      const payload = {
        address: selectedAccount.address,
        name: accountName,
        isDerived: isDerived
      }
      const result = dispatch(WalletPageActions.updateAccountName(payload))
      return result ? onClose() : setUpdateError(true)
    }
  }, [selectedAccount, accountName, dispatch, onClose])

  const generateQRData = React.useCallback(() => {
    if (selectedAccount) {
      generateQRCode(selectedAccount.address).then(qr => {
        if (isMounted) {
          setQRCode(qr)
        }
      })
    }
  }, [selectedAccount, isMounted])

  const onShowPrivateKey = async () => {
    if (!password || !selectedAccount) { // require password to view key
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

    onViewPrivateKey(
      selectedAccount?.address ?? '',
      selectedAccount?.coin
    )
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

  const handlePasswordKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
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
      return AccountButtonOptions.find((option) => option.id === accountModalType)?.name ?? ''
    }
    return ''
  }, [accountModalType])

  const orb = React.useMemo(() => {
    if (selectedAccount) {
      return create({ seed: selectedAccount.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
    }
  }, [selectedAccount])

  // effects
  React.useEffect(() => {
    generateQRData()
  }, [])

  // render
  return (
    <PopupModal title={getLocale(modalTitle)} onClose={onClickClose}>
      <Line />
      <StyledWrapper>
        {accountModalType === 'deposit' &&
          <>
            <NameAndIcon>
              <AccountCircle orb={orb} />
              <AccountName>{selectedAccount?.name ?? ''}</AccountName>
            </NameAndIcon>
            <QRCodeWrapper src={qrCode} />
            <CopyTooltip text={selectedAccount?.address ?? ''}>
              <AddressButton>{selectedAccount?.address ?? ''}<CopyIcon /></AddressButton>
            </CopyTooltip>
            <VerticalSpacer space={20} />
          </>
        }
        {accountModalType === 'edit' &&
          <>
            <Input
              value={accountName}
              placeholder={getLocale('braveWalletAddAccountPlaceholder')}
              onChange={handleAccountNameChanged}
              onKeyDown={handleKeyDown}
            />
            {updateError &&
              <ErrorText>{getLocale('braveWalletAccountSettingsUpdateError')}</ErrorText>
            }
            <ButtonRow>
              <NavButton
                onSubmit={onSubmitUpdateName}
                disabled={!accountName}
                text={getLocale('braveWalletAccountSettingsSave')}
                buttonType='secondary'
              />
            </ButtonRow>
          </>
        }
        {accountModalType === 'privateKey' &&
          <PrivateKeyWrapper>
            <WarningWrapper>
              <WarningText>{getLocale('braveWalletAccountSettingsDisclaimer')}</WarningText>
            </WarningWrapper>
            {privateKey
              ? <>
                {selectedAccount?.coin === BraveWallet.CoinType.FIL &&
                  <WarningWrapper>
                    <WarningText>
                      {filPrivateKeyFormatDescriptionTextParts.beforeTag}
                      <a target='_blank' href={FILECOIN_FORMAT_DESCRIPTION_URL} rel='noopener noreferrer'>
                        {filPrivateKeyFormatDescriptionTextParts.duringTag}
                      </a>
                      {filPrivateKeyFormatDescriptionTextParts.afterTag}
                    </WarningText>
                  </WarningWrapper>
                }
                <CopyTooltip text={privateKey}>
                  <PrivateKeyBubble>{privateKey}</PrivateKeyBubble>
                </CopyTooltip>
              </>
              : <PasswordInput
                placeholder={getLocale('braveWalletEnterYourBraveWalletPassword')}
                onChange={onPasswordChange}
                hasError={!!password && !isCorrectPassword}
                error={getLocale('braveWalletLockScreenError')}
                autoFocus={false}
                value={password}
                onKeyDown={handlePasswordKeyDown}
              />
            }
            <ButtonWrapper>
              <NavButton
                onSubmit={!privateKey ? onShowPrivateKey : onHidePrivateKey}
                text={getLocale(!privateKey
                  ? 'braveWalletAccountSettingsShowKey'
                  : 'braveWalletAccountSettingsHideKey'
                )}
                buttonType='primary'
                disabled={
                  privateKey
                    ? false
                    : password ? !isCorrectPassword : true
                }
              />
            </ButtonWrapper>
          </PrivateKeyWrapper>
        }
      </StyledWrapper>
    </PopupModal>
  )
}

export default AccountSettingsModal
