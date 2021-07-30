import styled from 'styled-components'
import DownIcon from '../../../assets/svg-icons/arrow-down-icon.svg'

export const StyledWrapper = styled.div`
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
`

export const ArrowDownIcon = styled.div`
  width: 14px;
  height: 14px;
  background-image: url(${DownIcon});
`

export const ArrowButton = styled.button`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  width: 14px;
  height: 14px;
  padding: 0px;
  margin-bottom: 12px;
`
