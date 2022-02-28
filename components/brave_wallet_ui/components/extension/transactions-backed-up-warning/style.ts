import styled from 'styled-components'
import { LoaderIcon } from 'brave-ui/components/icons'
import { WalletButton } from '../../shared/style'

interface StyleProps {
  needsMargin: boolean
}

export const StyledWrapper = styled.div`
  display: flex;
  width: 100%;
  height: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
`

export const Title = styled.span`
  font-family: Poppins;
  font-size: 15px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
  margin-bottom: 10px;
`

export const Description = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.text02};
  width: 80%;
  text-align: center;
  margin-bottom: 6px;
`

export const LearnMoreButton = styled(WalletButton) <Partial<StyleProps>>`
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  font-family: Poppins;
  font-size: 14px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.interactive05};
  margin: 0px;
  margin-bottom: ${(p) => p.needsMargin ? '15px' : '0px'};
  padding: 0px;
`

export const CheckboxRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  margin: 20px 0px;
  width: 70%;
`

export const CheckboxText = styled.span`
  font-size: 8px;
  line-height: 20px;
  color: ${(p) => p.theme.color.text02};
`

export const ClearButton = styled(WalletButton)`
  display: flex;
  flex-direction: row;
  justify-content: center;
  align-items: center;
  padding: 7px 22px;
  border: none;
  box-sizing: border-box;
  border-radius: 40px;
  background-color: ${(p) => p.theme.color.errorBorder};
  opacity: .4;
`

export const ClearButtonText = styled.span`
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  color: ${(p) => p.theme.palette.white};
  margin-right: 4px;
`

export const LoadIcon = styled(LoaderIcon)`
  color: ${p => p.theme.palette.white};
  height: 25px;
  width: 25px;
`
