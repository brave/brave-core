import styled from 'styled-components'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
`
export const AssetDescriptionWrapper = styled.div`
  margin-top: 6px;
`

export const AssetDescription = styled.p`
  font-family: Poppins;
  font-weight: 400;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${p => p.theme.color.text02};
  margin: 0;
`
