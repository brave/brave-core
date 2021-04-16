import styled from 'styled-components'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  width: 100%;
`

export const TopRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: space-between;
  width: 100%;
`

export const BalanceTitle = styled.span`
  font-family: Poppins;
	font-size: 15px;
	font-weight: normal;
	color: ${(p) => p.theme.palette.neutral600};
`

export const BalanceText = styled.span`
  font-family: Poppins;
  font-size: 32px;
  font-weight: 600;
  margin-bottom: 64px;
  color: ${(p) => p.theme.palette.neutral900};
`

export const ButtonRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: flex-start;
  width: 100%;
  margin-top: 8px;
`
