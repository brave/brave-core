import * as React from 'react'
import Radio from 'brave-ui/components/formControls/radio'

import { getLocale } from '../../../../common/locale'
import { NavButton, Panel } from '../'

// Styled Components
import {
  StyledWrapper,
  FormColumn,
  Input,
  ButtonRow,
  Description,
  AllowanceTitle,
  AllowanceContent,
  AllowanceOption
} from './style'
import Amount from '../../../utils/amount'
import { MAX_UINT256 } from '../../../common/constants/magics'

export enum MaxPriorityPanels {
  setSuggested = 0,
  setCustom = 1
}

type AllowanceTypes =
  | 'proposed'
  | 'custom'

export interface Props {
  onCancel: () => void
  onSave: (allowance: string) => void
  proposedAllowance: string
  symbol: string
  decimals: number
  approvalTarget: string
}

const EditAllowance = (props: Props) => {
  const [allowanceType, setAllowanceType] = React.useState<AllowanceTypes>('proposed')
  const [customAllowance, setCustomAllowance] = React.useState<string>('')

  const {
    onCancel,
    onSave,
    proposedAllowance,
    approvalTarget,
    symbol,
    decimals
  } = props

  const toggleAllowanceRadio = (key: AllowanceTypes) => {
    setAllowanceType(key)
  }

  const onChangeCustomAllowance = (event: React.ChangeEvent<HTMLInputElement>) => {
    setCustomAllowance(event.target.value)
  }

  const onClickSave = () => {
    onSave(allowanceType === 'custom' ? customAllowance : proposedAllowance)
    onCancel()
  }

  const isSaveButtonDisabled = React.useMemo(() => {
    return allowanceType === 'custom' && customAllowance === ''
  }, [allowanceType, customAllowance])

  const formattedProposedAllowance = React.useMemo(() => {
    const wrappedAmount = new Amount(proposedAllowance)
    return wrappedAmount.multiplyByDecimals(decimals).eq(MAX_UINT256)
      ? getLocale('braveWalletTransactionApproveUnlimited')
      : proposedAllowance
  }, [proposedAllowance])

  return (
    <Panel
      navAction={onCancel}
      title={getLocale('braveWalletEditPermissionsTitle')}
    >
      <StyledWrapper>
        <Description>
          {getLocale('braveWalletEditPermissionsDescription').replace('$1', approvalTarget)}
        </Description>
        <FormColumn>
          <Radio
            value={{
              proposed: allowanceType === 'proposed',
              custom: allowanceType === 'custom'
            }}
            onChange={toggleAllowanceRadio}
          >
            <div data-value='proposed'>
              <AllowanceOption>
                <AllowanceTitle>
                  {getLocale('braveWalletEditPermissionsProposedAllowance')}
                </AllowanceTitle>
                <AllowanceContent>
                  {formattedProposedAllowance} {symbol}
                </AllowanceContent>
              </AllowanceOption>
            </div>
            <div data-value='custom'>
              <AllowanceOption>
                <AllowanceTitle>
                  {getLocale('braveWalletEditPermissionsCustomAllowance')}
                </AllowanceTitle>
                <Input
                  placeholder={`0 ${symbol}`}
                  type='number'
                  value={customAllowance}
                  onChange={onChangeCustomAllowance}
                />
              </AllowanceOption>
            </div>
          </Radio>
        </FormColumn>

        <ButtonRow>
          <NavButton
            buttonType='secondary'
            text={getLocale('braveWalletBackupButtonCancel')}
            onSubmit={onCancel}
          />
          <NavButton
            buttonType='primary'
            text={getLocale('braveWalletAccountSettingsSave')}
            onSubmit={onClickSave}
            disabled={isSaveButtonDisabled}
          />
        </ButtonRow>
      </StyledWrapper>
    </Panel>
  )
}

export default EditAllowance
