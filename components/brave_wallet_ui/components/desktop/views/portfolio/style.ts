import styled from 'styled-components'
import { CaratStrongLeftIcon, ArrowUpIcon } from 'brave-ui/components/icons'

interface StyleProps {
  icon: string
  isDown: boolean
}

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
`

export const TopRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  width: 100%;
`

export const BalanceTitle = styled.span`
  font-family: Poppins;
  font-size: 15px;
  font-weight: normal;
  color: ${(p) => p.theme.palette.neutral600};
`

export const BalanceText = styled.span`
  font-family: Poppins;
  font-size: 32px;
  font-weight: 600;
  margin-bottom: 64px;
  color: ${(p) => p.theme.palette.neutral900};
`

export const PriceText = styled.span`
  font-family: Poppins;
  font-size: 24px;
  font-weight: 600;
  line-height: 36px;
  letter-spacing: 0.02em;
  margin-right: 10px;
`

export const ButtonRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  margin: 20px 0px;
`

// Will use brave-ui button comp in the future!
// Currently is missing "tiny" variant
export const BackButton = styled.button`
  display: flex;;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border-radius: 48px;
  padding: 3px 14px;
  border: ${(p) => `1px solid ${p.theme.palette.neutral600}`};
  font-family: Poppins;
  font-weight: 600;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.neutral900};
`

export const BackIcon = styled(CaratStrongLeftIcon)`
  width: auto;
  height: 14px;
  margin-right: 8px;
  color: ${(p) => p.theme.palette.neutral600};
`

export const InfoColumn = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  margin-left: 10px;
  margin-top: 10px;
`

export const AssetRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  margin-bottom: 9px;
`

export const PriceRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
`

export const AssetNameText = styled.span`
  font-family: Poppins;
  font-size: 20px;
  line-height: 30px;
  font-weight: 600;
  letter-spacing: 0.02em;
  color: ${(p) => p.theme.palette.neutral900};
`

export const DetailText = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  letter-spacing: 0.01em;
  font-weight: 400;
  color: ${(p) => p.theme.palette.neutral600};
`

export const AssetIcon = styled.div<Partial<StyleProps>>`
  width: 40px;
  height: 40px;
  border-radius: 100%;
  background: ${(p) => `url(${p.icon})`};
  margin-right: 12px;
`

export const SubDivider = styled.div`
  width: 100%;
  height: 2px;
  background-color: ${(p) => p.theme.palette.grey200};
  margin-bottom: 12px;
`

export const DividerText = styled.span`
  font-family: Poppins;
  font-size: 15px;
  line-height: 20px;
  letter-spacing: 0.04em;
  font-weight: 600;
  margin-bottom: 10px;
  color: ${(p) => p.theme.palette.neutral600};
`

export const PercentBubble = styled.div<Partial<StyleProps>>`
  display: flex;
  align-items: center;
  justify-conent: center;
  flex-direction: row;
  padding: 4px 8px;
  border-radius: 8px;
  background-color: ${(p) => p.isDown ? p.theme.palette.red600 : p.theme.palette.teal600};
`

export const PercentText = styled.span`
  font-family: Poppins;
  font-size: 11px;
  line-height: 17px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.white};
`

export const ArrowIcon = styled(ArrowUpIcon) <Partial<StyleProps>>`
  width: 12px;
  height: 12px;
  margin-right: 2px;
  transform: ${(p) => p.isDown ? 'rotate(270deg)' : 'rotate(90deg)'};
  color: ${(p) => p.theme.palette.white};
`
