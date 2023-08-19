// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Types
import { BraveWallet } from '../../../constants/types'

// Utils
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import { getLocale, splitStringForTag } from '../../../../common/locale'

// Components
import { NavButton } from '../buttons/nav-button/index'
import { PanelTab } from '../panel-tab/index'
import { CreateSiteOrigin } from '../../shared/create-site-origin/index'

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

import { TabRow, URLText } from '../shared-panel-styles'

// Hooks
import { useAddressOrb } from '../../../common/hooks/use-orb'

export interface Props {
  panelType: 'request' | 'read'
  accounts: BraveWallet.AccountInfo[]
  selectedNetwork?: BraveWallet.NetworkInfo
  encryptionKeyPayload: BraveWallet.GetEncryptionPublicKeyRequest
  decryptPayload: BraveWallet.DecryptRequest
  onProvideOrAllow: () => void
  onCancel: () => void
}

export function EncryptionKeyPanel (props: Props) {
  const {
    panelType,
    accounts,
    selectedNetwork,
    encryptionKeyPayload,
    decryptPayload,
    onProvideOrAllow,
    onCancel
  } = props
  const [isDecrypted, setIsDecrypted] = React.useState<boolean>(false)
  const payloadAddress = panelType === 'request' ? encryptionKeyPayload.address : decryptPayload.address
  const originInfo = panelType === 'request' ? encryptionKeyPayload.originInfo : decryptPayload.originInfo

  const foundAccountName = React.useMemo(() => {
    return accounts.find((account) => account.address.toLowerCase() === payloadAddress.toLowerCase())?.name
  }, [payloadAddress])

  const orb = useAddressOrb(payloadAddress)

  const onDecryptMessage = () => {
    setIsDecrypted(true)
  }

  const descriptionString = getLocale('braveWalletProvideEncryptionKeyDescription').replace('$url', originInfo.originSpec)
  const { duringTag, afterTag } = splitStringForTag(descriptionString)

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText>{selectedNetwork?.chainName ?? ''}</NetworkText>
      </TopRow>
      <AccountCircle orb={orb} />
      <AccountNameText>{reduceAccountDisplayName(foundAccountName ?? '', 14)}</AccountNameText>
      {panelType === 'read' &&
        <URLText>
          <CreateSiteOrigin
            originSpec={originInfo.originSpec}
            eTldPlusOne={originInfo.eTldPlusOne}
          />
        </URLText>
      }
      <PanelTitle>
        {panelType === 'request'
          ? getLocale('braveWalletProvideEncryptionKeyTitle')
          : getLocale('braveWalletReadEncryptedMessageTitle')}
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
              ? <>
                <CreateSiteOrigin
                  originSpec={duringTag ?? ''}
                  eTldPlusOne={originInfo.eTldPlusOne}
                />
                {afterTag}
              </>
              : decryptPayload.unsafeMessage}
          </MessageText>
        )}
      </MessageBox>
      <ButtonRow>
        <NavButton
          buttonType='secondary'
          text={getLocale('braveWalletButtonCancel')}
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
