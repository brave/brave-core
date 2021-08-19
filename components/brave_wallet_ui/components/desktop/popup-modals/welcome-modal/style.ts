import styled from 'styled-components'
import AllSetIcon from '../../../../assets/svg-icons/all-set-icon.svg'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
  padding: 0px 30px 30px 30px;
`

export const Title = styled.span`
  font-family: Poppins;
  font-size: 20px;
  font-weight: 600;
  line-height: 30px;
  letter-spacing: 0.02em;
  color: ${(p) => p.theme.color.text01};
  margin-bottom: 16px;
`

export const Description = styled.span`
  width: 306px;
  font-family: Poppins;
  font-size: 16px;
  line-height: 24px;
  text-align: center;
  color: ${(p) => p.theme.color.text02};
  margin-bottom: 32px;
`
export const ModalIcon = styled.div`
  width: 221px;
  height: 120px;
  background: url(${AllSetIcon});
  margin-bottom: 16px;
`
