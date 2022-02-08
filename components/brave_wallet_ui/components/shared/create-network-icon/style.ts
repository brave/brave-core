import styled from 'styled-components'
import { AssetIconFactory, AssetIconProps } from '../style'

interface StyleProps {
  marginRight: number
  isTestnet: boolean
  orb: string
}

export const IconWrapper = styled.div<Partial<StyleProps>>`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  width: 15px;
  height: 15px;
  margin-right: ${(p) => `${p.marginRight}px`};
  filter: ${(p) => p.isTestnet ? 'grayscale(100%)' : 'none'};
`

export const Placeholder = styled.div<Partial<StyleProps>>`
  width: 10px;
  height: 10px;
  border-radius: 100%;
  background-image: url(${(p) => p.orb});
  background-size: cover;
`

export const NetworkIcon = AssetIconFactory<AssetIconProps>({
  width: '15px',
  height: 'auto'
})
