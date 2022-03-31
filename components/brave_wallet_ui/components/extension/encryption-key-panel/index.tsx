import * as React from 'react'

// Types
import { BraveWallet, WalletAccountType } from '../../../constants/types'

// Utils
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import { getLocale } from '../../../../common/locale'

// Components
import { create } from 'ethereum-blockies'
import { NavButton, PanelTab } from '..'

// Styled Components
import {
  StyledWrapper,
  AccountCircle,
  AccountNameText,
  TopRow,
  NetworkText,
  PanelTitle,
  MessageBox,
  MessageText,
  ButtonRow,
  DecryptButton
} from './style'

import { TabRow } from '../shared-panel-styles'

export interface Props {
  panelType: 'request' | 'read'
  accounts: WalletAccountType[]
  selectedNetwork: BraveWallet.NetworkInfo
  encryptionKeyPayload: BraveWallet.GetEncryptionPublicKeyRequest
  onProvideOrAllow: () => void
  onCancel: () => void
}

function EncryptionKeyPanel (props: Props) {
  const {
    panelType,
    accounts,
    selectedNetwork,
    encryptionKeyPayload,
    onProvideOrAllow,
    onCancel
  } = props
  const [isDecrypted, setIsDecrypted] = React.useState<boolean>(false)

  const foundAccountName = React.useMemo(() => {
    return accounts.find((account) => account.address.toLowerCase() === encryptionKeyPayload.address.toLowerCase())?.name
  }, [encryptionKeyPayload])

  const orb = React.useMemo(() => {
    return create({ seed: encryptionKeyPayload.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [encryptionKeyPayload])

  const onDecryptMessage = () => {
    setIsDecrypted(true)
  }

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText>{selectedNetwork.chainName}</NetworkText>
      </TopRow>
      <AccountCircle orb={orb} />
      <AccountNameText>{reduceAccountDisplayName(foundAccountName ?? '', 14)}</AccountNameText>
      <PanelTitle>
        {panelType === 'request'
          ? getLocale('braveWalletProvideEncryptionKeyTitle')
          : getLocale('braveWalletReadEncryptedMessageTitle').replace('$1', encryptionKeyPayload.origin.url)}
      </PanelTitle>
      <TabRow>
        <PanelTab
          isSelected={true}
          text={getLocale('braveWalletSignTransactionMessageTitle')}
        />
      </TabRow>
      <MessageBox
        needsCenterAlignment={panelType === 'read' && !isDecrypted}
      >
        {panelType === 'read' && !isDecrypted ? (
          <DecryptButton
            onClick={onDecryptMessage}
          >
            {getLocale('braveWalletReadEncryptedMessageDecryptButton')}
          </DecryptButton>
        ) : (
          <MessageText>
            {panelType === 'request'
              ? getLocale('braveWalletProvideEncryptionKeyDescription').replace('$1', encryptionKeyPayload.origin.url)
              : encryptionKeyPayload.message}
          </MessageText>
        )}
      </MessageBox>
      <ButtonRow>
        <NavButton
          buttonType='secondary'
          text={getLocale('braveWalletBackupButtonCancel')}
          onSubmit={onCancel}
        />
        <NavButton
          buttonType='primary'
          text={
            panelType === 'request'
              ? getLocale('braveWalletProvideEncryptionKeyButton')
              : getLocale('braveWalletReadEncryptedMessageButton')
          }
          onSubmit={onProvideOrAllow}
        />
      </ButtonRow>
    </StyledWrapper>
  )
}

export default EncryptionKeyPanel
