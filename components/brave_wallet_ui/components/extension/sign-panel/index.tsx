import * as React from 'react'
import { create } from 'ethereum-blockies'
import { WalletAccountType, EthereumChain } from '../../../constants/types'
import { reduceAccountDisplayName } from '../../../utils/reduce-account-name'
import { getLocale } from '../../../../common/locale'
import { NavButton, PanelTab } from '../'
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
  WarningBox,
  WarningText,
  WarningTitle,
  WarningTitleRow,
  WarningIcon,
  LearnMoreButton
} from './style'
import { TabRow } from '../shared-panel-styles'

export interface Props {
  selectedAccount: WalletAccountType
  selectedNetwork: EthereumChain
  message: string
  onSign: () => void
  onCancel: () => void
  showWarning: boolean
}

enum SignDataSteps {
  SignRisk = 0,
  SignData = 1
}

function SignPanel (props: Props) {
  const {
    selectedAccount,
    selectedNetwork,
    message,
    onSign,
    onCancel,
    showWarning
  } = props
  const [signStep, setSignStep] = React.useState<SignDataSteps>(SignDataSteps.SignData)
  const orb = React.useMemo(() => {
    return create({ seed: selectedAccount.address.toLowerCase(), size: 8, scale: 16 }).toDataURL()
  }, [selectedAccount.address])

  React.useMemo(() => {
    if (showWarning) {
      setSignStep(SignDataSteps.SignRisk)
    }
  }, [showWarning])

  const onContinueSigning = () => {
    setSignStep(SignDataSteps.SignData)
  }

  const onClickLearnMore = () => {
    window.open('https://support.brave.com/hc/en-us/articles/4409513799693', '_blank')
  }

  return (
    <StyledWrapper>
      <TopRow>
        <NetworkText>{selectedNetwork.chainName}</NetworkText>
      </TopRow>
      <AccountCircle orb={orb} />
      <AccountNameText>{reduceAccountDisplayName(selectedAccount.name, 14)}</AccountNameText>
      <PanelTitle>{getLocale('braveWalletSignTransactionTitle')}</PanelTitle>
      {signStep === SignDataSteps.SignRisk &&
        <WarningBox>
          <WarningTitleRow>
            <WarningIcon />
            <WarningTitle>{getLocale('braveWalletSignWarningTitle')}</WarningTitle>
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
          <MessageBox>
            <MessageText>{message}</MessageText>
          </MessageBox>
        </>
      }
      <ButtonRow>
        <NavButton
          buttonType='secondary'
          text={getLocale('braveWalletBackupButtonCancel')}
          onSubmit={onCancel}
        />
        <NavButton
          buttonType={signStep === SignDataSteps.SignData ? 'sign' : 'danger'}
          text={signStep === SignDataSteps.SignData ? getLocale('braveWalletSignTransactionButton') : getLocale('braveWalletButtonContinue')}
          onSubmit={signStep === SignDataSteps.SignRisk ? onContinueSigning : onSign}
        />
      </ButtonRow>
    </StyledWrapper>
  )
}

export default SignPanel
