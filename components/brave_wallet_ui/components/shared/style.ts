import styled from 'styled-components'
import { BraveWallet } from '../../constants/types'
import transparent40x40Image from '../../assets/png-icons/transparent40x40.png'
import { stripERC20TokenImageURL } from '../../utils/string-utils'

export interface AssetIconProps {
  icon?: string
}

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

export const WalletButton = styled.button`
  &:focus-visible {
    outline-style: solid;
    outline-color: ${p => p.theme.palette.blurple300};
    outline-width: 2px;
  }
 `

interface StyleProps {
  status: BraveWallet.TransactionStatus
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
    : p.status === BraveWallet.TransactionStatus.Rejected || p.status === BraveWallet.TransactionStatus.Error ? p.theme.color.errorBorder
      : p.status === BraveWallet.TransactionStatus.Unapproved ? p.theme.color.interactive08 : p.theme.color.warningIcon
  };
  margin-right: 6px;
`
