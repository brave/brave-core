import * as React from 'react'

// Types
import { BraveWallet, WalletAccountType } from '../../../constants/types'
import { SignMessagePayload } from '../../../panel/constants/action_types'

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
  MessageText,
  ButtonRow,
  WarningTitleRow,
  WarningIcon
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
  URLText
} from '../shared-panel-styles'
// import { Row } from '../../shared/style'

export interface Props {
  accounts: WalletAccountType[]
  defaultNetworks: BraveWallet.NetworkInfo[]
  selectedNetwork: BraveWallet.NetworkInfo
  signMessageData: BraveWallet.SignMessageRequest[]
  onSign: () => void
  onCancel: () => void
  showWarning: boolean
}

enum SignDataSteps {
  SignRisk = 0,
  SignData = 1
}

const onClickLearnMore = () => {
  window.open('https://support.brave.com/hc/en-us/articles/4409513799693', '_blank')
}

export const SignPanel = (props: Props) => {
  const {
    accounts,
    defaultNetworks,
    selectedNetwork,
    signMessageData,
    onSign,
    onCancel,
    showWarning
  } = props

  // state
  const [signStep, setSignStep] = React.useState<SignDataSteps>(SignDataSteps.SignData)
  const [selectedQueueData, setSelectedQueueData] = React.useState<SignMessagePayload>(signMessageData[0])
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

  const network = React.useMemo(() => {
    return defaultNetworks.find((n) => n.coin === signMessageData[0].coin) ?? selectedNetwork
  }, [defaultNetworks, selectedNetwork, signMessageData])

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
        <NetworkText>{network.chainName}</NetworkText>
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
              text={getLocale('braveWalletSignTransactionMessageTitle')}
            />
          </TabRow>

          {hasUnicode(selectedQueueData.message) &&
            <WarningBox warningType='warning'>
              <WarningTitleRow>
                <WarningIcon color={'warningIcon'} />
                <WarningTitle warningType='warning'>
                  {
                    'Non-ASCII characters detected!' // TODO
                  }
                </WarningTitle>
              </WarningTitleRow>
              <LearnMoreButton
                onClick={() => setRenderUnicode(prev => !prev)}
              >
                {
                 renderUnicode
                  ? 'View decoded message' // TODO: Locale
                  : 'View encoded message' // TODO: Locale
                  // getLocale('braveWalletAllowAddNetworkLearnMoreButton')
                }
              </LearnMoreButton>
            </WarningBox>
          }

          <MessageBox>
            <MessageText>
              {
                renderUnicode
                  ? selectedQueueData.message
                  : unicodeEscape(selectedQueueData.message)
              }
            </MessageText>
          </MessageBox>
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
