import styled from 'styled-components'
import { CaratCircleODownIcon } from 'brave-ui/components/icons'
import { WalletButton } from '../../shared/style'

interface StyleProps {
  isPanel?: boolean
}

export const CaratDownIcon = styled(CaratCircleODownIcon) <StyleProps>`
  width: 14px;
  height: 14px;
  margin-left: 4px;
  color: ${(p) => p.isPanel ? p.theme.palette.white : p.theme.color.interactive07};
`

export const OvalButton = styled(WalletButton)<StyleProps>`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: ${(p) => p.onClick ? 'pointer' : 'text'};
  outline: none;
  background: none;
  border-radius: 48px;
  padding: 3px 10px;
  border: 1px solid ${(p) => p.isPanel ? 'rgba(255,255,255,0.5)' : p.theme.color.interactive08};
`

export const OvalButtonText = styled.span<StyleProps>`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  letter-spacing: 0.01em;
  color: ${(p) => p.isPanel ? p.theme.palette.white : p.theme.color.text02};
  font-weight: 600;
`
