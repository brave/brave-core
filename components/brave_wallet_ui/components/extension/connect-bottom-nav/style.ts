import styled from 'styled-components'

export const StyledWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: space-evenly;
  width: 100%;
  min-height: 100px;
`

export const ButtonRow = styled.div`
  display: flex;
  flex-direction: row;
  align-items: center;
  justify-content: center;
  width: 100%;
`

export const DisclaimerText = styled.span`
	font-family: Poppins;
	font-size: 11px;
	line-height: 17px;
	font-weight: 400;
  color: ${(p) => p.theme.color.text02};
`
