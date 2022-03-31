import styled from 'styled-components'
import { LoaderIcon } from 'brave-ui/components/icons'

export interface StyleProps {
  alignment: 'right' | 'left'
}

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  margin-bottom: 24px;
`

export const TopRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
  margin-bottom: 20px;
  gap: 10px;
`

export const AssetsColumnWrapper = styled.div`
  display: inline-flex;
  flex-direction: row;
  align-items: center;
  justify-content: left;
`

export const AssetsColumnItemSpacer = styled.div`
  display: inline-flex;
  align-items: 'center';
  justify-content: 'center';
  margin-right: 19px;
`
export const TextWrapper = styled.div<StyleProps>`
  display: flex;
  justify-content: ${p => p.alignment === 'right' ? 'flex-end' : 'flex-start'};
  width: 100%;
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

export const LoadIcon = styled(LoaderIcon)`
  color: ${p => p.theme.color.interactive08};
  height: 70px;
  width: 70px;
  opacity: .4;
`

export const LoadIconWrapper = styled.div`
  display: flex;
  width: 100%;
  height: 100%;
  flex-direction: column;
  align-items: center;
  justify-content: center;
`
