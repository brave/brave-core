import styled from 'styled-components'

export const StyledWrapper = styled.div`
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
`

export const FaucetWrapper = styled.div`
  display: flex;
  width: 100%;
  flex-direction: column;
  align-items: flex-start;
  justify-content: center;
`

export const FaucetTitle = styled.span`
  font-family: Poppins;
  font-size: 20px;
  font-weight: 600;
  line-height: 30px;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.02em;
`

export const FaucetDescription = styled.span`
  font-family: Poppins;
  font-size: 13px;
  line-height: 20px;
  font-weight: 300;
  color: ${(p) => p.theme.color.text02};
  margin-bottom: 20px;
`
