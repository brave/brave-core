import styled from 'styled-components'

interface StyleProps {
  buttonType: 'primary' | 'secondary'
  disabled?: boolean
}

// Will need to change to brave-ui button

export const StyledButton = styled.button<StyleProps>`
	display: flex;
	align-items: center;
	justify-content: center;
	cursor: pointer;
	border-radius: 40px;
	padding: 10px 22px;
  outline: none;
	background-color: ${(p) =>
    p.disabled ? p.theme.color.disabled : p.buttonType === 'primary' ? p.theme.palette.blurple500 : 'transparent'};
	border: ${(p) =>
    p.buttonType === 'primary'
      ? 'none'
      : `1px solid ${p.theme.color.interactive08}`};
	margin-right: ${(p) => (p.buttonType === 'primary' ? '0px' : '8px')};
`

export const ButtonText = styled.span<Partial<StyleProps>>`
	font-size: 13px;
	font-weight: 600;
	line-height: 20px;
	color: ${(p) =>
    p.buttonType === 'primary' ? p.theme.palette.white : p.theme.color.interactive07};
`
