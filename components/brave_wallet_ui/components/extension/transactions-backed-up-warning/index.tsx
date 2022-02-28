import * as React from 'react'

// Components
import { NavButton } from '../'
import { Checkbox } from 'brave-ui'
import { getLocale } from '../../../../common/locale'

// Styled Components
import {
  StyledWrapper,
  Title,
  Description,
  LearnMoreButton,
  CheckboxRow,
  ClearButton,
  ClearButtonText,
  LoadIcon,
  CheckboxText
} from './style'

export interface Props {
  onContinue: (clearTransactions: boolean) => void
  duplicateNonceTranscations: boolean
}

function TransactionsBackedUpWarning (props: Props) {
  const { onContinue, duplicateNonceTranscations } = props
  const [clearTransactions, setClearTransactions] = React.useState<boolean>(false)
  const [isClearing, setIsClearing] = React.useState<boolean>(false)

  const onCheckClearTransactions = (key: string, selected: boolean) => {
    if (key === 'clearTransactions') {
      setClearTransactions(selected)
    }
  }

  const onClickContinue = () => {
    if (clearTransactions) {
      setIsClearing(true)
    }
    onContinue(clearTransactions)
  }

  const onClickLearnMore = () => {
    chrome.tabs.create({ url: 'https://support.brave.com/hc/en-us/articles/4537540021389' }, () => {
      if (chrome.runtime.lastError) {
        console.error('tabs.create failed: ' + chrome.runtime.lastError.message)
      }
    })
  }

  return (
    <StyledWrapper>
      <Title>{getLocale('braveWalletBackedUpTransactionsTitle')}</Title>
      <Description>{getLocale('braveWalletBackedUpTransactionsDescription')}</Description>
      <LearnMoreButton
        needsMargin={duplicateNonceTranscations}
        onClick={onClickLearnMore}
      >
        {getLocale('braveWalletWelcomePanelButton')}
      </LearnMoreButton>
      {!duplicateNonceTranscations &&
        <CheckboxRow>
          <Checkbox
            children={<CheckboxText data-key='clearTransactions'>{getLocale('braveWalletBackedUpTransactionsCheckbox')}</CheckboxText>}
            size='small'
            value={{ clearTransactions: clearTransactions }}
            onChange={onCheckClearTransactions}
          />
        </CheckboxRow>
      }
      {isClearing ? (
        <ClearButton>
          <ClearButtonText>
            {getLocale('braveWalletBackedUpTransactionsClearingButton')}
          </ClearButtonText>
          <LoadIcon />
        </ClearButton>
      ) : (
        <NavButton
          disabled={isClearing}
          buttonType={
            clearTransactions
              ? 'danger'
              : 'primary'
          }
          text={
            duplicateNonceTranscations
              ? getLocale('braveWalletBackedUpTransactionsOKButton')
              : clearTransactions
                ? getLocale('braveWalletBackedUpTransactionsClearButton')
                : getLocale('braveWalletButtonContinue')
          }
          onSubmit={onClickContinue}
        />
      )
      }
    </StyledWrapper >
  )
}

export default TransactionsBackedUpWarning
