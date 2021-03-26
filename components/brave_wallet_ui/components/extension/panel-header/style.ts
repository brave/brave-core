import styled from 'styled-components'
import CloseIcon from '../assets/close.svg'

export const HeaderTitle = styled.span`
  font-family: Poppins;
  font-size: 18px;
  line-height: 26px;
  font-weight: 600;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.palette.neutral700};
  text-transform: capitalize;
`

export const HeaderWrapper = styled.div`
  display: flex;
  height: 54px;
  width: 100%;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  border-bottom: ${(p) => `1px solid ${p.theme.color.separatorLine}`};
  padding: 0px 12px;
`

export const CloseButton = styled.button`
  display: flex;;
  align-items: center;
  justify-content: center;
  cursor: pointer;
  width: 20px;
  height: 20px;
  background: url(${CloseIcon});
  outline: none;
  border: none;
`
