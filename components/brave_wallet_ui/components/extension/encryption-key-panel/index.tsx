// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Types
import { BraveWallet, SerializableDecryptRequest, SerializableGetEncryptionPublicKeyRequest, WalletAccountType } from '../../../constants/types'

// Utils
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import { getLocale, splitStringForTag } from '../../../../common/locale'

// Components
import { create } from 'ethereum-blockies'
import { NavButton, PanelTab } from '..'
import { CreateSiteOrigin } from '../../shared'

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

export interface Props {
  panelType: 'request' | 'read'
  accounts: WalletAccountType[]
  selectedNetwork?: BraveWallet.NetworkInfo
  encryptionKeyPayload: SerializableGetEncryptionPublicKeyRequest
  eTldPlusOne: string
  decryptPayload: SerializableDecryptRequest
  onProvideOrAllow: () => void
  onCancel: () => void
}

function EncryptionKeyPanel (props: Props) {
  const {
    panelType,
    accounts,
    selectedNetwork,
    encryptionKeyPayload,
    eTldPlusOne,
    decryptPayload,
    onProvideOrAllow,
    onCancel
  } = props
  const [isDecrypted, setIsDecrypted] = React.useState<boolean>(false)
  const payloadAddress = panelType === 'request' ? encryptionKeyPayload.address : decryptPayload.address

  const foundAccountName = React.useMemo(() => {
    return accounts.find((account) => account.address.toLowerCase() === payloadAddress.toLowerCase())?.name
  }, [payloadAddress])

  const orb = React.useMemo(() => {
    return create({ seed: payloadAddress.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [payloadAddress])

  const onDecryptMessage = () => {
    setIsDecrypted(true)
  }

  const descriptionString = getLocale('braveWalletProvideEncryptionKeyDescription').replace('$url', encryptionKeyPayload.originInfo.originSpec)
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
            originSpec={encryptionKeyPayload.originInfo.originSpec}
            // TODO(apaymyshev): why originSpec is coming from payload, but eTldPlusOne from props?
            eTldPlusOne={eTldPlusOne}
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
                  eTldPlusOne={eTldPlusOne}
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
