import styled from 'styled-components'

export const StyledWrapper = styled.div`
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
`

export const Spacer = styled.div`
  margin-bottom: 30px;
`

export const NetworkNotSupported = styled.p`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 14px;
  line-height: 20px;
  text-align: center;
  letter-spacing: 0.01em;
  color: ${p => p.theme.color.text02};
  margin: 55px 20px 59px;
  padding: 0;
`
