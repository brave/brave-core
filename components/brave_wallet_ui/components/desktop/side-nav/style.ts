import styled from 'styled-components'

export const StyledWrapper = styled.div`
	display: flex;
	flex-direction: column;
	align-items: center;
	justify-content: center;
	width: 180px;
`

export const AccountsDivider = styled.div`
  width: 90%;
  height: 2px;
  background-color: ${(p) => p.theme.color.divider01};
  margin: 12px 0px;
`
