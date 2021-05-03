import styled from 'styled-components'

export const StyledWrapper = styled.div`
	display: flex;
	flex-direction: column;
	align-items: center;
	justify-content: center;
	width: 100%;
	padding-top: 28px;
`

export const URLText = styled.span`
	font-family: Poppins;
	font-style: normal;
	font-weight: normal;
	font-size: 13px;
	line-height: 20px;
	text-align: center;
	letter-spacing: 0.01em;
	margin-bottom: 11px;
  color: ${(p) => p.theme.color.text02};
`

export const PanelTitle = styled.span`
	font-family: Poppins;
	font-style: normal;
	font-weight: 600;
	font-size: 15px;
	line-height: 20px;
	text-align: center;
	letter-spacing: 0.04em;
  color: ${(p) => p.theme.color.text01};
`

// This is temp, will be replaced with real FavIcon
export const FavIcon = styled.div`
	width: 44px;
	height: 44px;
	border-radius: 5px;
	background-color: ${(p) => p.theme.palette.grey200};
	margin-bottom: 7px;
`
