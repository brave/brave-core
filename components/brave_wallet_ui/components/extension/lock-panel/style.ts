import styled from 'styled-components'
import LockIcon from '../../../assets/svg-icons/graphic-lock-icon.svg'

export const StyledWrapper = styled.div`
  display: flex;
  height: 100%;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  background-color: ${(p) => p.theme.color.background01};
`

export const Title = styled.span`
  font-family: Poppins;
  font-size: 15px;
  font-weight: 600;
  line-height: 20px;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.04em;
  margin-bottom: 12px;
`

export const IconBackground = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  width: 162px;
  height: 162px;
  border-radius: 100%;
  background-color: ${(p) => p.theme.color.background01};
  margin-bottom: 24px;
`

export const PageIcon = styled.div`
  width: 103px;
  height: 88px;
  background: url(${LockIcon});
`

export const Column = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  width: 250px;
  margin-bottom: 8px;
`
