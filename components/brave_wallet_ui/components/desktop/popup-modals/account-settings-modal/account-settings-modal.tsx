// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as qr from 'qr-image'

// utils
import { reduceAddress } from '../../../../utils/reduce-address'
import { getLocale, getLocaleWithTag } from '../../../../../common/locale'

// constants
import { FILECOIN_FORMAT_DESCRIPTION_URL } from '../../../../common/constants/urls'

// types
import {
  AccountSettingsNavTypes,
  BraveWallet,
  WalletAccountType,
  UpdateAccountNamePayloadType,
  TopTabNavObjectType
} from '../../../../constants/types'

// options
import {
  AccountSettingsNavOptions,
  HardwareAccountSettingsNavOptions
} from '../../../../options/account-settings-nav-options'

// components
import { NavButton } from '../../../extension'
import { CopyTooltip } from '../../../shared/copy-tooltip/copy-tooltip'
import TopTabNav from '../../top-tab-nav/index'
import PopupModal from '../index'
import PasswordInput from '../../../shared/password-input/index'

// hooks
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
  InputLabelText
} from './account-settings-modal.style'

interface Props {
  onClose: () => void
  onUpdateAccountName: (payload: UpdateAccountNamePayloadType) => { success: boolean }
  onChangeTab: (id: AccountSettingsNavTypes) => void
  onRemoveAccount: (address: string, hardware: boolean, coin: BraveWallet.CoinType) => void
  onViewPrivateKey: (address: string, isDefault: boolean, coin: BraveWallet.CoinType) => void
  onDoneViewingPrivateKey: () => void
  onToggleNav: () => void
  privateKey: string
  hideNav: boolean
  tab: AccountSettingsNavTypes
  title: string
  account: WalletAccountType
}

export const AccountSettingsModal = ({
  title,
  account,
  tab,
  hideNav,
  privateKey,
  onClose,
  onToggleNav,
  onUpdateAccountName,
  onChangeTab,
  onRemoveAccount,
  onViewPrivateKey,
  onDoneViewingPrivateKey
}: Props) => {
  // state
  const [accountName, setAccountName] = React.useState<string>(account.name)
  const [showPrivateKey, setShowPrivateKey] = React.useState<boolean>(false)
  const [updateError, setUpdateError] = React.useState<boolean>(false)
  const [password, setPassword] = React.useState<string>('')
  const [isCorrectPassword, setIsCorrectPassword] = React.useState<boolean>(true)
  const [qrCode, setQRCode] = React.useState<string>('')

  // custom hooks
  const { keyringService } = useApiProxy()

  // methods
  const handleAccountNameChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    setAccountName(event.target.value)
    setUpdateError(false)
  }

  const onSubmitUpdateName = () => {
    const isDerived = account?.accountType === 'Primary'
    const payload = {
      address: account.address,
      name: accountName,
      isDerived: isDerived
    }
    onUpdateAccountName(payload).success ? onClose() : setUpdateError(true)
  }

  const generateQRData = () => {
    const image = qr.image(account.address)
    let chunks: Uint8Array[] = []
    image
      .on('data', (chunk: Uint8Array) => chunks.push(chunk))
      .on('end', () => {
        setQRCode(`data:image/png;base64,${Buffer.concat(chunks).toString('base64')}`)
      })
  }

  const onSelectTab = (id: AccountSettingsNavTypes) => {
    setShowPrivateKey(false)
    onChangeTab(id)
  }

  const removeAccount = () => {
    onRemoveAccount(account.address, false, account.coin)
    onToggleNav()
    onClose()
  }

  const onShowPrivateKey = async () => {
    if (!password) { // require password to view key
      return
    }

    // entered password must be correct
    const {
      result: isPasswordValid
    } = await keyringService.validatePassword(password)

    if (!isPasswordValid) {
      setIsCorrectPassword(isPasswordValid) // set or clear error
      return // need valid password to continue
    }

    // clear entered password & error
    setPassword('')
    setIsCorrectPassword(true)

    if (onViewPrivateKey) {
      const isDefault = account?.accountType === 'Primary'
      onViewPrivateKey(account?.address ?? '', isDefault, account?.coin)
    }
    setShowPrivateKey(true)
  }

  const onHidePrivateKey = () => {
    onDoneViewingPrivateKey()
    setShowPrivateKey(false)
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

  // memos / computed
  const tabList = React.useMemo((): TopTabNavObjectType[] => {
    return account.accountType === 'Trezor' ||
      account.accountType === 'Ledger'
      ? HardwareAccountSettingsNavOptions()
      : AccountSettingsNavOptions()
  }, [account])

  const filPrivateKeyFormatDescriptionTextParts = getLocaleWithTag(
    'braveWalletFilExportPrivateKeyFormatDescription'
  )

  // effects
  React.useEffect(() => {
    generateQRData()
  })

  // render
  return (
    <PopupModal title={title} onClose={onClickClose}>
      {!hideNav &&
        <TopTabNav
          tabList={tabList}
          onSelectTab={onSelectTab}
          selectedTab={tab}
        />
      }
      <StyledWrapper>
        {tab === 'details' &&
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
            <QRCodeWrapper src={qrCode} />
            <CopyTooltip text={account.address}>
              <AddressButton>{reduceAddress(account.address)}<CopyIcon /></AddressButton>
            </CopyTooltip>
            <ButtonRow>
              <NavButton
                onSubmit={onSubmitUpdateName}
                disabled={!accountName}
                text={getLocale('braveWalletAccountSettingsSave')}
                buttonType='secondary'
              />
              {account?.accountType === 'Secondary' &&
                <NavButton
                  onSubmit={removeAccount}
                  text={getLocale('braveWalletAccountSettingsRemove')}
                  buttonType='danger'
                />
              }
            </ButtonRow>
          </>
        }
        {tab === 'privateKey' &&
          <PrivateKeyWrapper>
            <WarningWrapper>
              <WarningText>{getLocale('braveWalletAccountSettingsDisclaimer')}</WarningText>
            </WarningWrapper>
            {showPrivateKey
              ? <>
                {account.coin === BraveWallet.CoinType.FIL &&
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
              : <>
                <InputLabelText>{getLocale('braveWalletEnterYourPassword')}</InputLabelText>
                <PasswordInput
                  placeholder={getLocale('braveWalletCreatePasswordInput')}
                  onChange={onPasswordChange}
                  hasError={!!password && !isCorrectPassword}
                  error={getLocale('braveWalletLockScreenError')}
                  autoFocus={false}
                  value={password}
                  onKeyDown={handlePasswordKeyDown}
                />
              </>
            }
            <ButtonWrapper>
              <NavButton
                onSubmit={!showPrivateKey ? onShowPrivateKey : onHidePrivateKey}
                text={!showPrivateKey ? getLocale('braveWalletAccountSettingsShowKey') : getLocale('braveWalletAccountSettingsHideKey')}
                buttonType='primary'
                disabled={
                  showPrivateKey
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
