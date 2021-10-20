import styled from 'styled-components'
import MMIcon from '../../../../assets/svg-icons/onboarding/import-from-metamask.svg'
import BIcon from '../../../../assets/svg-icons/onboarding/reset-to-brave-wallet.svg'

interface StyleProps {
  needsNewPassword: boolean
  useSamePasswordVerified: boolean
}

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  padding-top: 32px;
`

export const Title = styled.span`
  font-family: Poppins;
  font-size: 20px;
  font-weight: 600;
  line-height: 30px;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.02em;
  margin-bottom: 10px;
`

export const Description = styled.span`
  display: flex;
  align-items: center;
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  font-weight: 300;
  color: ${(p) => p.theme.color.text02};
  max-width: 380px;
  text-align: center;
  margin-bottom: 24px;
`

export const InputColumn = styled.div<Partial<StyleProps>>`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  width: 250px;
  margin-bottom: ${(p) => p.useSamePasswordVerified ? '0px' : '25px'};
`

export const MetaMaskIcon = styled.div`
  width: 240px;
  height: 160px;
  background: url(${MMIcon});
  background-size: 100%;
  background-repeat: no-repeat;
  margin-bottom: 18px;
`

export const BraveIcon = styled.div<Partial<StyleProps>>`
  width: 226px;
  height: 160px;
  background: url(${BIcon});
  margin-bottom: 25px;
  background-repeat: no-repeat;
  background-size: 100%;
`

export const LostButton = styled.button`
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  font-family: Poppins;
  font-style: normal;
  font-weight: 500;
  font-size: 13px;
  line-height: 19px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text03};
  margin-top: 35px;
  max-width: 340px;
`

export const PasswordTitle = styled.span<Partial<StyleProps>>`
  width: 295px;
  text-align: center;
  font-family: Poppins;
  font-size: 15px;
  font-weight: 600;
  line-height: 20px;
  color: ${(p) => p.needsNewPassword ? p.theme.color.errorText : p.theme.color.text02};
  letter-spacing: 0.04em;
  margin-bottom: ${(p) => p.needsNewPassword ? '12px' : '6px'};
`

export const CheckboxRow = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: flex-start;
  flex-direction: row;
  margin-bottom: 6px;
  width: 250px;
`
