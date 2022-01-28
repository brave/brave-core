import styled from 'styled-components'

import StarActiveIcon from '../../assets/svg-icons/star-active-icon.svg'
import StarIcon from '../../assets/svg-icons/star-icon.svg'

export interface StyleProps {
  active: boolean
}

export const StyledWrapper = styled.div`
  display: flex;
  align-items: center;
  width: 17px;
  height: 17px;
`

export const Star = styled.button<StyleProps>`
  width: 100%;
  height: 100%;
  background-repeat: no-repeat;
  background-size: contain;
  background-position: center center;
  background-image: url(${p => p.active ? StarActiveIcon : StarIcon});
  background-color: transparent;
  border: none;
  box-shadow: none;
  cursor: pointer;
`
