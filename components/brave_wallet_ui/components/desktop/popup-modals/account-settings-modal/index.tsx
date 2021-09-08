import * as React from 'react'
import * as qr from 'qr-image'
import {
  AccountSettingsNavTypes,
  WalletAccountType,
  UpdateAccountNamePayloadType
} from '../../../../constants/types'
import {
  PopupModal,
  TopTabNav
} from '../..'
import {
  AccountSettingsNavOptions
} from '../../../../options/account-settings-nav-options'
import { reduceAddress } from '../../../../utils/reduce-address'
import { copyToClipboard } from '../../../../utils/copy-to-clipboard'
import { NavButton } from '../../../extension'
import { Tooltip } from '../../../shared'
import locale from '../../../../constants/locale'

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
  ErrorText
} from './style'

export interface Props {
  onClose: () => void
  onUpdateAccountName: (payload: UpdateAccountNamePayloadType) => { success: boolean }
  onCopyToClipboard: () => void
  onChangeTab: (id: AccountSettingsNavTypes) => void
  onRemoveAccount: (address: string) => void
  onViewPrivateKey: (address: string, isDefault: boolean) => void
  onDoneViewingPrivateKey: () => void
  onToggleNav: () => void
  privateKey: string
  hideNav: boolean
  tab: AccountSettingsNavTypes
  title: string
  account: WalletAccountType
}

const AddAccountModal = (props: Props) => {
  const {
    title,
    account,
    tab,
    hideNav,
    privateKey,
    onClose,
    onToggleNav,
    onUpdateAccountName,
    onCopyToClipboard,
    onChangeTab,
    onRemoveAccount,
    onViewPrivateKey,
    onDoneViewingPrivateKey
  } = props
  const [accountName, setAccountName] = React.useState<string>(account.name)
  const [showPrivateKey, setShowPrivateKey] = React.useState<boolean>(false)
  const [updateError, setUpdateError] = React.useState<boolean>(false)
  const [qrCode, setQRCode] = React.useState<string>('')

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
    let chunks: Array<Uint8Array> = []
    image
      .on('data', (chunk: Uint8Array) => chunks.push(chunk))
      .on('end', () => {
        setQRCode(`data:image/png;base64,${Buffer.concat(chunks).toString('base64')}`)
      })
  }

  React.useEffect(() => {
    generateQRData()
  })

  const changeTab = (id: AccountSettingsNavTypes) => {
    onChangeTab(id)
  }

  const removeAccount = () => {
    onRemoveAccount(account.address)
    onToggleNav()
    onClose()
  }

  const onShowPrivateKey = () => {
    if (onViewPrivateKey) {
      const isDefault = account?.accountType === 'Primary'
      onViewPrivateKey(account?.address ?? '', isDefault)
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

  const onCopyPrivateKey = async () => {
    if (privateKey) {
      await copyToClipboard(privateKey)
    }
  }

  return (
    <PopupModal title={title} onClose={onClickClose}>
      {!hideNav &&
        <TopTabNav
          tabList={AccountSettingsNavOptions}
          onSubmit={changeTab}
          selectedTab={tab}
        />
      }
      <StyledWrapper>
        {tab === 'details' &&
          <>
            <Input
              value={accountName}
              placeholder={locale.addAccountPlaceholder}
              onChange={handleAccountNameChanged}
            />
            {updateError &&
              <ErrorText>{locale.accountSettingsUpdateError}</ErrorText>
            }
            <QRCodeWrapper src={qrCode} />
            <Tooltip text={locale.toolTipCopyToClipboard}>
              <AddressButton onClick={onCopyToClipboard}>{reduceAddress(account.address)}<CopyIcon /></AddressButton>
            </Tooltip>
            <ButtonRow>
              <NavButton
                onSubmit={onSubmitUpdateName}
                disabled={!accountName}
                text={locale.accountSettingsSave}
                buttonType='secondary'
              />
              {account?.accountType === 'Secondary' &&
                <NavButton
                  onSubmit={removeAccount}
                  text={locale.accountSettingsRemove}
                  buttonType='danger'
                />
              }
            </ButtonRow>
          </>
        }
        {tab === 'privateKey' &&
          <PrivateKeyWrapper>
            <WarningWrapper>
              <WarningText>{locale.accountSettingsDisclaimer}</WarningText>
            </WarningWrapper>
            {showPrivateKey &&
              <Tooltip text={locale.toolTipCopyToClipboard}>
                <PrivateKeyBubble onClick={onCopyPrivateKey}>{privateKey}</PrivateKeyBubble>
              </Tooltip>
            }
            <ButtonWrapper>
              <NavButton
                onSubmit={showPrivateKey === false ? onShowPrivateKey : onHidePrivateKey}
                text={showPrivateKey === false ? locale.accountSettingsShowKey : locale.accountSettingsHideKey}
                buttonType='primary'
              />
            </ButtonWrapper>
          </PrivateKeyWrapper>
        }
      </StyledWrapper>
    </PopupModal>
  )
}

export default AddAccountModal
