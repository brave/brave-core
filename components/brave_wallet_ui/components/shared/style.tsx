import styled from 'styled-components'

// types
import { BraveWallet } from '../../constants/types'

// utils
import { stripERC20TokenImageURL } from '../../utils/string-utils'

// images & icons
import transparent40x40Image from '../../assets/png-icons/transparent40x40.png'
import EyeOnIcon from '../../assets/svg-icons/eye-on-icon.svg'
import EyeOffIcon from '../../assets/svg-icons/eye-off-icon.svg'

export interface AssetIconProps {
  icon?: string
}

interface StyleProps {
  status: BraveWallet.TransactionStatus
  space: number
}

export const StatusBubble = styled.div<Partial<StyleProps>>`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 10px;
  height: 10px;
  border-radius: 100%;
  opacity: ${(p) => p.status === BraveWallet.TransactionStatus.Submitted ||
    p.status === BraveWallet.TransactionStatus.Approved ||
    p.status === BraveWallet.TransactionStatus.Unapproved
    ? 0.4
    : 1
  };
  background-color: ${(p) => p.status === BraveWallet.TransactionStatus.Confirmed || p.status === BraveWallet.TransactionStatus.Approved
    ? p.theme.color.successBorder
    : p.status === BraveWallet.TransactionStatus.Rejected || p.status === BraveWallet.TransactionStatus.Error || p.status === BraveWallet.TransactionStatus.Dropped ? p.theme.color.errorBorder
      : p.status === BraveWallet.TransactionStatus.Unapproved ? p.theme.color.interactive08 : p.theme.color.warningIcon
  };
  margin-right: 6px;
`

export const ErrorText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  color: ${(p) => p.theme.color.errorText};
  margin-bottom: 10px;
`

export const AssetIconFactory = styled.img.attrs<AssetIconProps>(props => ({
  src: stripERC20TokenImageURL(props.icon)
    ? props.icon

    // Display theme background (using a transparent image) if no icon to
    // render.
    : transparent40x40Image,

  // Defer loading the image until it reaches a calculated distance from the
  // viewport, as defined by the browser.
  //
  // Ref: https://web.dev/browser-level-image-lazy-loading
  loading: 'lazy'
}))

export const WalletButton = styled.button<{
  isDraggedOver?: boolean
}>`
  &:focus-visible {
    outline-style: solid;
    outline-color: ${p => p.theme.palette.blurple300};
    outline-width: 2px;
  }
 `

export const VerticalSpacer = styled.div<{ space: number | string }>`
  display: flex;
  height: ${p => typeof p.space === 'number' ? `${p.space}px` : p.space};
`

export const ToggleVisibilityButton = styled(WalletButton)<{
  isVisible: boolean
}>`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  cursor: pointer;
  outline: none;
  background: none;
  border: none;
  padding: 0px;
  width: 18px;
  height: 18px;
  background-color: ${(p) => p.theme.color.text02};
  -webkit-mask-image: url(${(p) => p.isVisible ? EyeOffIcon : EyeOnIcon});
  mask-image: url(${(p) => p.isVisible ? EyeOffIcon : EyeOnIcon});
  mask-size: contain;
  mask-position: center;
  mask-repeat: no-repeat;
`
