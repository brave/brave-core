import styled from 'styled-components'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: row;
  height: 40px;

`

export const AssetIcon = styled.img`
  width: 40px;
  height: auto;
  margin-right: 12px;
`

export const NameAndSymbolWrapper = styled.div`
  display: flex;
  flex-direction: column;
  justify-content: space-between;
  height: 100%;
  padding: 1px 0;
`

export const AssetName = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  letter-spacing: 0.01em;
  color: ${p => p.theme.color.text01};
`

export const AssetSymbol = styled(AssetName)`
  color: ${p => p.theme.color.text03};
  text-transform: uppercase;
`
