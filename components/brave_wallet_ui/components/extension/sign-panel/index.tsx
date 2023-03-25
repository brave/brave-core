// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'

// Types
import { BraveWallet, SerializableSignMessageRequest, WalletAccountType } from '../../../constants/types'

// Utils
import { getLocale } from '../../../../common/locale'
import { unicodeEscape, hasUnicode } from '../../../utils/string-utils'

// Components
import { NavButton, PanelTab } from '../'
import { CreateSiteOrigin } from '../../shared'
import { create } from 'ethereum-blockies'

// Styled Components
import {
  StyledWrapper,
  AccountCircle,
  AccountNameText,
  TopRow,
  NetworkText,
  PanelTitle,
  MessageBox,
  MessageHeader,
  MessageText,
  ButtonRow,
  WarningTitleRow
} from './style'

import {
  QueueStepRow,
  QueueStepButton,
  QueueStepText
} from '../confirm-transaction-panel/common/style'

import {
  TabRow,
  WarningBox,
  WarningTitle,
  WarningText,
  LearnMoreButton,
  URLText,
  WarningIcon
} from '../shared-panel-styles'
import { useGetDefaultNetworksQuery } from '../../../common/slices/api.slice'

export interface Props {
  accounts: WalletAccountType[]
  selectedNetwork?: BraveWallet.NetworkInfo
  signMessageData: SerializableSignMessageRequest[]
  onSign: () => void
  onCancel: () => void
  showWarning: boolean
}

enum SignDataSteps {
  SignRisk = 0,
  SignData = 1
}

const onClickLearnMore = () => {
  chrome.tabs.create({
    url: 'https://support.brave.com/hc/en-us/articles/4409513799693'
  }, () => {
    if (chrome.runtime.lastError) {
      console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
    }
  })
}

export const SignPanel = (props: Props) => {
  const {
    accounts,
    selectedNetwork,
    signMessageData,
    onSign,
    onCancel,
    showWarning
  } = props

  // queries
  const { data: defaultNetworks = [] } = useGetDefaultNetworksQuery()

  // state
  const [signStep, setSignStep] = React.useState<SignDataSteps>(SignDataSteps.SignData)
  const [selectedQueueData, setSelectedQueueData] = React.useState<SerializableSignMessageRequest>(signMessageData[0])
  const [renderUnicode, setRenderUnicode] = React.useState<boolean>(true)

  // memos
  const orb = React.useMemo(() => {
    return create({ seed: selectedQueueData.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [selectedQueueData.address])

  const signMessageQueueInfo = React.useMemo(() => {
    return {
      queueLength: signMessageData.length,
      queueNumber: signMessageData.findIndex((data) => data.id === selectedQueueData.id) + 1
    }
  }, [signMessageData, selectedQueueData])

  const isDisabled = React.useMemo((): boolean => signMessageData.findIndex(
    (data) =>
      data.id === selectedQueueData.id) !== 0
    , [signMessageData, selectedQueueData]
  )

  const network = React.useMemo(
    () =>
      defaultNetworks.find((n) => n.coin === signMessageData[0].coin) ??
      selectedNetwork,
    [defaultNetworks, selectedNetwork, signMessageData[0].coin]
  )

  // methods
  const findAccountName = (address: string) => {
    return accounts.find((account) => account.address.toLowerCase() === address.toLowerCase())?.name
  }

  const onContinueSigning = () => {
    setSignStep(SignDataSteps.SignData)
  }

  const onQueueNextSignMessage = () => {
    if (signMessageQueueInfo.queueNumber === signMessageQueueInfo.queueLength) {
      setSelectedQueueData(signMessageData[0])
      return
    }
    setSelectedQueueData(signMessageData[signMessageQueueInfo.queueNumber])
  }

  // effects
  React.useEffect(() => {
    setSelectedQueueData(signMessageData[0])
  }, [signMessageData])

  React.useEffect(() => {
    if (showWarning) {
      setSignStep(SignDataSteps.SignRisk)
    }
  }, [showWarning])

  // render
  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText>{network?.chainName ?? ''}</NetworkText>
        {signMessageQueueInfo.queueLength > 1 &&
          <QueueStepRow>
            <QueueStepText>{signMessageQueueInfo.queueNumber} {getLocale('braveWalletQueueOf')} {signMessageQueueInfo.queueLength}</QueueStepText>
            <QueueStepButton
              onClick={onQueueNextSignMessage}
            >
              {signMessageQueueInfo.queueNumber === signMessageQueueInfo.queueLength
                ? getLocale('braveWalletQueueFirst')
                : getLocale('braveWalletQueueNext')
              }
            </QueueStepButton>
          </QueueStepRow>
        }
      </TopRow>
      <AccountCircle orb={orb} />
      <URLText>
        <CreateSiteOrigin
          originSpec={selectedQueueData.originInfo.originSpec}
          eTldPlusOne={selectedQueueData.originInfo.eTldPlusOne}
        />
      </URLText>
      <AccountNameText>{findAccountName(selectedQueueData.address) ?? ''}</AccountNameText>
      <PanelTitle>{getLocale('braveWalletSignTransactionTitle')}</PanelTitle>
      {signStep === SignDataSteps.SignRisk &&
        <WarningBox warningType='danger'>
          <WarningTitleRow>
            <WarningIcon />
            <WarningTitle warningType='danger'>{getLocale('braveWalletSignWarningTitle')}</WarningTitle>
          </WarningTitleRow>
          <WarningText>{getLocale('braveWalletSignWarning')}</WarningText>
          <LearnMoreButton onClick={onClickLearnMore}>{getLocale('braveWalletAllowAddNetworkLearnMoreButton')}</LearnMoreButton>
        </WarningBox>
      }
      {signStep === SignDataSteps.SignData &&
        <>
          <TabRow>
            <PanelTab
              isSelected={true}
              text={
                selectedQueueData.isEip712
                  ? getLocale('braveWalletSignTransactionEIP712MessageTitle')
                  : getLocale('braveWalletSignTransactionMessageTitle')
              }
            />
          </TabRow>

          {(hasUnicode(selectedQueueData.message) ||
              (selectedQueueData.isEip712 && hasUnicode(selectedQueueData.domain))) &&
            <WarningBox warningType='warning'>
              <WarningTitleRow>
                <WarningIcon color={'warningIcon'} />
                <WarningTitle warningType='warning'>
                  {
                    getLocale('braveWalletNonAsciiCharactersInMessageWarning')
                  }
                </WarningTitle>
              </WarningTitleRow>
              <LearnMoreButton
                onClick={() => setRenderUnicode(prev => !prev)}
              >
                {
                 renderUnicode
                  ? getLocale('braveWalletViewDecodedMessage')
                  : getLocale('braveWalletViewEncodedMessage')
                }
              </LearnMoreButton>
            </WarningBox>
          }

          {selectedQueueData.isEip712 && (
            <MessageBox height='180px'>
              <MessageHeader>
                {getLocale('braveWalletSignTransactionEIP712MessageDomain')}:
              </MessageHeader>
              <MessageText>
                {!renderUnicode && hasUnicode(selectedQueueData.domain)
                  ? unicodeEscape(selectedQueueData.domain)
                  : selectedQueueData.domain
                }
              </MessageText>

              <MessageHeader>
                {getLocale('braveWalletSignTransactionMessageTitle')}:
              </MessageHeader>
              <MessageText>
                {!renderUnicode && hasUnicode(selectedQueueData.message)
                  ? unicodeEscape(selectedQueueData.message)
                  : selectedQueueData.message
                }
              </MessageText>
            </MessageBox>
          )}

          {!selectedQueueData.isEip712 && (
            <MessageBox>
              <MessageText>
                {!renderUnicode && hasUnicode(selectedQueueData.message)
                  ? unicodeEscape(selectedQueueData.message)
                  : selectedQueueData.message
                }
              </MessageText>
            </MessageBox>
          )}
        </>
      }
      <ButtonRow>
        <NavButton
          buttonType='secondary'
          text={getLocale('braveWalletButtonCancel')}
          onSubmit={onCancel}
          disabled={isDisabled}
        />
        <NavButton
          buttonType={signStep === SignDataSteps.SignData ? 'sign' : 'danger'}
          text={signStep === SignDataSteps.SignData ? getLocale('braveWalletSignTransactionButton') : getLocale('braveWalletButtonContinue')}
          onSubmit={signStep === SignDataSteps.SignRisk ? onContinueSigning : onSign}
          disabled={isDisabled}
        />
      </ButtonRow>
    </StyledWrapper>
  )
}

export default SignPanel
