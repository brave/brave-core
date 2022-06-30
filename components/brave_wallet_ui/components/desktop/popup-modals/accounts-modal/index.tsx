// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import * as React from 'react'
import * as qr from 'qr-image'

// Redux
import {
  useDispatch,
  useSelector
} from 'react-redux'

// Actions
import {
  AccountsTabState,
  AccountsTabActions
} from '../../../../page/reducers/accounts-tab-reducer'
import { WalletPageActions } from '../../../../page/actions'

// Types
import {
  BraveWallet,
  PageState
} from '../../../../constants/types'

// Components
import { AccountButtonOptions } from '../../../../options/account-buttons'
import { PopupModal } from '../..'
import { NavButton } from '../../../extension'
import { Tooltip } from '../../../shared'

// Utils
import { FILECOIN_FORMAT_DESCRIPTION_URL } from '../../../../common/constants/urls'
import { reduceAddress } from '../../../../utils/reduce-address'
import { copyToClipboard } from '../../../../utils/copy-to-clipboard'
import { getLocale, getLocaleWithTag } from '../../../../../common/locale'

// Hooks
import { useCopy } from '../../../../common/hooks'

// Styled Components
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
  Line
} from './style'

const AccountsModal = () => {
  // redux
  const dispatch = useDispatch()
  const {
    selectedAccount,
    accountModalType
  } = useSelector((state: { accountsTab: AccountsTabState }) => state.accountsTab)
  const privateKey = useSelector(({ page }: { page: PageState }) => page.privateKey)

  // state
  const [accountName, setAccountName] = React.useState<string>(selectedAccount?.name ?? '')
  const [showPrivateKey, setShowPrivateKey] = React.useState<boolean>(false)
  const [updateError, setUpdateError] = React.useState<boolean>(false)
  const [qrCode, setQRCode] = React.useState<string>('')

  // custom hooks
  const { copyText } = useCopy()

  // methods
  const onCopyToClipboard = React.useCallback(async () => {
    if (selectedAccount) {
      await copyText(selectedAccount.address)
    }
  }, [selectedAccount])

  const handleAccountNameChanged = (event: React.ChangeEvent<HTMLInputElement>) => {
    setAccountName(event.target.value)
    setUpdateError(false)
  }

  const onClose = () => {
    dispatch(AccountsTabActions.setShowAccountModal(false))
    dispatch(AccountsTabActions.setSelectedAccount(undefined))
  }

  const onUpdateAccountName = React.useCallback(() => {
    if (selectedAccount) {
      const isDerived = selectedAccount.accountType === 'Primary'
      const result = dispatch(WalletPageActions.updateAccountName({ address: selectedAccount.address, name: accountName, isDerived }))
      return result ? onClose() : setUpdateError(true)
    }
  }, [selectedAccount, accountName])

  const generateQRData = React.useCallback(() => {
    if (selectedAccount) {
      const image = qr.image(selectedAccount.address)
      let chunks: Uint8Array[] = []
      image
        .on('data', (chunk: Uint8Array) => chunks.push(chunk))
        .on('end', () => {
          setQRCode(`data:image/png;base64,${Buffer.concat(chunks).toString('base64')}`)
        })
    }
  }, [selectedAccount])

  const removeAccount = React.useCallback(() => {
    if (selectedAccount) {
      if (selectedAccount.accountType === 'Ledger' || selectedAccount.accountType === 'Trezor') {
        dispatch(WalletPageActions.removeHardwareAccount({ address: selectedAccount.address, coin: selectedAccount.coin }))
        onClose()
        return
      }
      dispatch(WalletPageActions.removeImportedAccount({ address: selectedAccount.address, coin: selectedAccount.coin }))
      onClose()
    }
  }, [selectedAccount])

  const onShowPrivateKey = React.useCallback(() => {
    if (selectedAccount) {
      const isDefault = selectedAccount.accountType === 'Primary'
      dispatch(WalletPageActions.viewPrivateKey({ address: selectedAccount.address, isDefault, coin: selectedAccount.coin }))
      setShowPrivateKey(true)
    }
  }, [selectedAccount])

  const onHidePrivateKey = React.useCallback(() => {
    dispatch(WalletPageActions.doneViewingPrivateKey())
    setShowPrivateKey(false)
  }, [])

  const onClickClose = () => {
    onHidePrivateKey()
    setUpdateError(false)
    onClose()
  }

  const onCopyPrivateKey = async () => {
    if (privateKey) {
      await copyToClipboard(privateKey)
    }
  }

  const handleKeyDown = (event: React.KeyboardEvent<HTMLInputElement>) => {
    if (event.key === 'Enter' && accountName) {
      onUpdateAccountName()
    }
  }

  React.useEffect(() => {
    generateQRData()
  }, [selectedAccount])

  const filPrivateKeyFormatDescriptionTextParts =
    getLocaleWithTag('braveWalletFilExportPrivateKeyFormatDescription')

  // memos
  const modalTitle = React.useMemo((): string => {
    if (accountModalType) {
      return AccountButtonOptions.find((option) => option.id === accountModalType)?.name ?? ''
    }
    return ''
  }, [accountModalType])

  return (
    <PopupModal title={modalTitle} onClose={onClickClose}>
      <Line />
      <StyledWrapper>
        {accountModalType === 'deposit' &&
          <>
            <QRCodeWrapper src={qrCode} />
            <Tooltip text={getLocale('braveWalletToolTipCopyToClipboard')}>
              <AddressButton onClick={onCopyToClipboard}>{reduceAddress(selectedAccount?.address ?? '')}<CopyIcon /></AddressButton>
            </Tooltip>
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
                onSubmit={onUpdateAccountName}
                disabled={!accountName}
                text={getLocale('braveWalletAccountSettingsSave')}
                buttonType='secondary'
              />
              {selectedAccount?.accountType === 'Secondary' &&
                <NavButton
                  onSubmit={removeAccount}
                  text={getLocale('braveWalletAccountSettingsRemove')}
                  buttonType='danger'
                />
              }
            </ButtonRow>
          </>
        }
        {accountModalType === 'export' &&
          <PrivateKeyWrapper>
            <WarningWrapper>
              <WarningText>{getLocale('braveWalletAccountSettingsDisclaimer')}</WarningText>
            </WarningWrapper>
            {showPrivateKey && selectedAccount && selectedAccount.coin === BraveWallet.CoinType.FIL &&
              <WarningWrapper>
                <WarningText>
                  {filPrivateKeyFormatDescriptionTextParts.beforeTag}
                  <a target='_blank' href={FILECOIN_FORMAT_DESCRIPTION_URL}>
                    {filPrivateKeyFormatDescriptionTextParts.duringTag}
                  </a>
                  {filPrivateKeyFormatDescriptionTextParts.afterTag}
                </WarningText>
              </WarningWrapper>
            }
            {showPrivateKey &&
              <Tooltip text={getLocale('braveWalletToolTipCopyToClipboard')}>
                <PrivateKeyBubble onClick={onCopyPrivateKey}>{privateKey}</PrivateKeyBubble>
              </Tooltip>
            }
            <ButtonWrapper>
              <NavButton
                onSubmit={!showPrivateKey ? onShowPrivateKey : onHidePrivateKey}
                text={!showPrivateKey ? getLocale('braveWalletAccountSettingsShowKey') : getLocale('braveWalletAccountSettingsHideKey')}
                buttonType='primary'
              />
            </ButtonWrapper>
          </PrivateKeyWrapper>
        }
      </StyledWrapper>
    </PopupModal>
  )
}

export default AccountsModal
