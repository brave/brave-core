import styled from 'styled-components'

export interface StyleProps {
  alignment: 'right' | 'left' | 'center'
}

export const StyledWrapper = styled.div`
  display: flex;
  flex: 1;
  flex-direction: column;
  width: 100%;

  & > div.infinite-scroll-component {
    width: 100%;
  }
`

export const TableWrapper = styled.div`
  width: 100%;
`

export const AssetsColumnWrapper = styled.div`
  display: inline-flex;
  flex-direction: row;
  align-items: center;
  justify-content: left;
`

export const AssetsColumnItemSpacer = styled.div`
  display: inline-flex;
  align-items: center;
  justify-content: center;
  margin-right: 19px;
`
export const TextWrapper = styled.div<StyleProps>`
  display: flex;
  justify-content: ${p => {
    switch (p.alignment) {
      case 'left':
        return 'flex-start'
      case 'right':
        return 'flex-end'
      case 'center':
        return 'center'
      default:
        return 'center'
    }
  }};
  width: 100%;
  min-width: 30px;
  font-family: Poppins;
  font-size: 14px;
  letter-spacing: 0.01em;
`

export const LineChartWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: flex-end;
  height: 30px;
  max-width: 120px;
  margin: 0 auto;
`
export const LoadIconWrapper = styled.div`
  display: flex;
  justify-content: center;
`
export const CoinGeckoText = styled.span`
  font-family: Arial;
  font-size: 10px;
  font-weight: normal;
  color: ${(p) => p.theme.color.text03};
  margin: 15px 0px;
`
