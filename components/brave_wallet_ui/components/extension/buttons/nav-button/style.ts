import styled from 'styled-components'
import CloseIcon from '../../../../assets/svg-icons/close.svg'
import KeyIcon from '../../../../assets/svg-icons/key-icon.svg'
import CheckIcon from '../../assets/filled-checkmark.svg'
import { PanelButtonTypes } from './index'
import { WalletButton } from '../../../shared/style'
interface StyleProps {
  buttonType: PanelButtonTypes
  disabled?: boolean
  addTopMargin?: boolean
  fullWidth?: boolean
}

export const StyledButton = styled(WalletButton) <Partial<StyleProps>>`
  display: flex;
  align-items: center;
  justify-content: center;
  cursor: ${(p) => p.disabled ? 'default' : 'pointer'};
  border-radius: 40px;
  padding: 10px 22px;
  outline: none;
  margin-top: ${(p) => p.addTopMargin ? '8px' : '0px'};
  background-color: ${(p) =>
    p.disabled ? p.theme.color.disabled
      : p.buttonType === 'primary' ||
        p.buttonType === 'confirm' ||
        p.buttonType === 'sign' ? p.theme.palette.blurple500
        : p.buttonType === 'danger' ? p.theme.color.errorBorder
          : 'transparent'};
  border: ${(p) =>
    p.buttonType === 'secondary' ||
      p.buttonType === 'reject' ? `1px solid ${p.theme.color.interactive08}`
      : 'none'};
  margin-right: ${(p) =>
    p.buttonType === 'primary' ||
      p.buttonType === 'confirm' ||
      p.buttonType === 'sign' ? '0px' : '8px'};
  pointer-events: ${(p) => p.disabled ? 'none' : 'auto'};
`

export const ButtonText = styled.span<Partial<StyleProps>>`
  font-size: 13px;
  font-weight: 600;
  line-height: 20px;
  color: ${(p) =>
    p.buttonType === 'secondary' ||
      p.buttonType === 'reject' ? p.theme.color.interactive07
      : p.theme.palette.white};
`

export const RejectIcon = styled.div`
  width: 18px;
  height: 18px;
  background-color: ${(p) => p.theme.color.text02};
  -webkit-mask-image: url(${CloseIcon});
  mask-image: url(${CloseIcon});
  mask-size: 100%;
  margin-right: 10px;
`

export const SignIcon = styled.div`
  width: 18px;
  height: 18px;
  background-color: ${(p) => p.theme.palette.white};
  -webkit-mask-image: url(${KeyIcon});
  mask-image: url(${KeyIcon});
  mask-size: 100%;
  margin-right: 10px;
`

export const ConfirmIcon = styled.div`
  width: 18px;
  height: 18px;
  background-color: ${(p) => p.theme.palette.white};
  -webkit-mask-image: url(${CheckIcon});
  mask-image: url(${CheckIcon});
  mask-size: 100%;
  margin-right: 10px;
`
