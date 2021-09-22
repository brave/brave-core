import styled from 'styled-components'
import MMIcon from '../../../../assets/svg-icons/meta-mask-icon.svg'
import BIcon from '../../../../assets/svg-icons/brave-icon.svg'
import AIcon from '../../../../assets/svg-icons/import-arrow-icon.svg'

interface StyleProps {
  isMetaMask: boolean
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

export const PageIcons = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
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
  width: 94px;
  height: 94px;
  background: url(${MMIcon});
  margin-right: 4px;
`

export const BraveIcon = styled.div<Partial<StyleProps>>`
  width: ${(p) => p.isMetaMask ? '82px' : '118px'};
  height: ${(p) => p.isMetaMask ? '94px' : '135px'};
  background: url(${BIcon});
  margin-right: 4px;
  background-size: 100%;
`

export const ArrowIcon = styled.div`
  width: 61px;
  height: 15px;
  background: url(${AIcon});
  margin-right: 4px;
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
