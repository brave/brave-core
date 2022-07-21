import styled from 'styled-components'

export const NftImageIframe = styled.iframe`
  border: none;
  width: ${p => p.width ? p.width : '40px'};
  height: ${p => p.height ? p.height : '40px'};
`
