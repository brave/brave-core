// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

/* eslint-disable max-len */

import { FC } from 'react'
import styled, { css, CSSProperties } from 'styled-components'
import { Link } from 'react-router-dom'
import * as leo from '@brave/leo/tokens/css/variables'
import Icon from '@brave/leo/react/icon'
import Button from '@brave/leo/react/button'

// types
import { BraveWallet, StringWithAutocomplete } from '../../constants/types'
import IThemeProps from 'brave-ui/theme/theme-interface'

// utils
import { stripERC20TokenImageURL } from '../../utils/string-utils'

// components
import { LoaderIcon } from 'brave-ui/components/icons'

// images & icons
import transparent40x40Image from '../../assets/png-icons/transparent40x40.png'
import EyeOnIcon from '../../assets/svg-icons/eye-on-icon.svg'
import EyeOffIcon from '../../assets/svg-icons/eye-off-icon.svg'
import CheckmarkSvg from '../../assets/svg-icons/big-checkmark.svg'
import CloseSvg from '../../assets/svg-icons/close.svg'
import ClipboardSvg from '../../assets/svg-icons/copy-to-clipboard-icon.svg'
import DownloadSvg from '../../assets/svg-icons/download-icon.svg'
import CheckIconSvg from '../../assets/svg-icons/checkbox-check.svg'
import SwitchDown from '../../assets/svg-icons/switch-icon.svg'
import WarningCircleFilled from '../../assets/svg-icons/warning-circle-icon.svg'
import WarningTriangleFilled from '../../assets/svg-icons/warning-triangle-filled.svg'

export type ThemeColor = StringWithAutocomplete<keyof IThemeProps['color']>

// graphics
import BraveWalletWithCoins from '../../assets/svg-icons/onboarding/brave-wallet-with-coins.svg'
import { makePaddingMixin } from '../../utils/style.utils'

// re-export "send" styles
export { Text } from '../../page/screens/send/shared.styles'

// Spacers
export const VerticalSpacer = styled.div<{ space: number | string }>`
  display: flex;
  height: ${(p) => (typeof p.space === 'number' ? `${p.space}px` : p.space)};
`

// Text
export const LinkText = styled.a`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 14px;
  letter-spacing: 0.01em;
  color: ${(p) => p.theme.color.interactive05};
  margin: 0px;
  cursor: pointer;
  display: inline-flex;
  flex-direction: row;
  gap: 8px;
  justify-content: center;
  text-decoration: none;
`

export const MutedLinkText = styled(LinkText)`
  font-family: 'Inter', 'Poppins';
  font-size: 12px;
  font-weight: 400;
  color: ${leo.color.text.tertiary};
  line-height: 20px;
`

export const ErrorText = styled.span`
  font-family: Poppins;
  font-size: 12px;
  line-height: 18px;
  color: ${(p) => p.theme.color.errorText};
  margin-bottom: 10px;
`

type FlexProps = Partial<
  Pick<
    CSSProperties,
    'flex' | 'alignItems' | 'justifyContent' | 'gap' | 'alignSelf' | 'flexWrap'
  >
>

// Mixins
export const walletButtonFocusMixin = css`
  &:focus-visible {
    outline-style: solid;
    outline-color: ${(p) => p.theme.palette.blurple300};
    outline-width: 2px;
  }
`

/**
 * Also forces the scroll indicator to be visible on MacOS when present,
 * even when the element is not hovered
 */
export const styledScrollbarMixin = css`
  ::-webkit-scrollbar {
    appearance: none;
    -webkit-appearance: none;
  }

  ::-webkit-scrollbar:vertical {
    width: 7px;
  }

  ::-webkit-scrollbar:horizontal {
    height: 7px;
  }

  ::-webkit-scrollbar-thumb {
    border-radius: 4px;
    background-color: rgba(0, 0, 0, 0.5);
    box-shadow: 0 0 1px rgba(255, 255, 255, 0.5);
  }

  ::-webkit-scrollbar-track {
    background-color: none;
    border-radius: 8px;
  }
`

export const backgroundColorMixin = css<{
  color?: ThemeColor
}>`
  background-color: ${(p) =>
    p?.color
      ? p.theme.color?.[p.color] || p.theme.palette?.[p.color] || p.color
      : p.theme.palette.white};
`

// Containers
export const Row = styled.div<
  FlexProps & {
    maxWidth?: CSSProperties['maxWidth']
    minWidth?: CSSProperties['minWidth']
    minHeight?: CSSProperties['minHeight']
    height?: '100%' | 'unset'
    margin?: number | string
    padding?: number | string
    width?: CSSProperties['width']
    marginBottom?: number | string
    // https://styled-components.com/docs/api#transient-props
    $wrap?: boolean
    gap?: string
  }
>`
  cursor: ${(p) => (p.onClick ? 'pointer' : 'unset')};
  font-family: 'Poppins';
  display: flex;
  flex-direction: row;
  flex-wrap: ${(p) => (p.$wrap ? 'wrap' : 'unset')};
  flex: ${(p) => p.flex ?? 'unset'};
  align-items: ${(p) => p.alignItems ?? 'center'};
  align-self: ${(p) => p.alignSelf ?? 'unset'};
  justify-content: ${(p) => p.justifyContent ?? 'center'};
  gap: ${(p) => p.gap ?? 'unset'};
  width: ${(p) => p.width ?? '100%'};
  min-width: ${(p) => p.minWidth ?? 'unset'};
  max-width: ${(p) => p.maxWidth ?? 'unset'};
  height: ${(p) => p.height ?? 'unset'};
  min-height: ${(p) => p.minHeight ?? 'unset'};
  margin: ${(p) => p.margin ?? 'unset'};
  ${(p) =>
    p?.marginBottom || p?.marginBottom === 0
      ? css`
          margin-bottom: ${typeof p?.marginBottom === 'number'
            ? `${p.marginBottom}px`
            : p?.marginBottom || 'unset'};
        `
      : ''}
  position: relative;
  ${makePaddingMixin(0)}
  box-sizing: border-box;
  gap: ${(p) => p.gap ?? 'unset'};
  flex-wrap: ${(p) => p.flexWrap ?? 'unset'};
`

export const Column = styled.div<
  FlexProps & {
    width?: string
    height?: string
    fullWidth?: boolean
    fullHeight?: boolean
    color?: ThemeColor
    padding?: number | string
    margin?: number | string
  }
>`
  font-family: 'Poppins';
  height: ${(p) => (p.fullHeight ? '100%' : p?.height || 'unset')};
  width: ${(p) => (p.fullWidth ? '100%' : p?.width || 'unset')};
  flex: ${(p) => p.flex ?? 'unset'};
  display: flex;
  flex-direction: column;
  align-items: ${(p) => p.alignItems ?? 'center'};
  align-self: ${(p) => p.alignSelf ?? 'unset'};
  justify-content: ${(p) => p.justifyContent ?? 'center'};
  gap: ${(p) => p.gap ?? 'unset'};
  margin: ${(p) => p.margin ?? 0};
  ${(p) => p?.color && backgroundColorMixin};
  ${makePaddingMixin(0)}
  box-sizing: border-box;
`

export const ScrollableColumn = styled(Column)<{
  scrollDisabled?: boolean
  maxHeight?: string
  marginBottom?: string
}>`
  justify-content: flex-start;
  align-items: flex-start;
  max-height: ${(p) => p.maxHeight || '100%'};
  overflow-y: ${(p) => (p.scrollDisabled ? 'unset' : 'auto')};
  margin-bottom: ${(p) => p.marginBottom || 'unset'};
  width: 100%;
`

export const Flex = styled.div`
  flex: 1;
`

export const StatusBubble = styled.div<{
  status: BraveWallet.TransactionStatus
}>`
  display: flex;
  align-items: center;
  justify-content: center;
  width: 10px;
  height: 10px;
  border-radius: 100%;
  opacity: ${(p) =>
    p.status === BraveWallet.TransactionStatus.Submitted ||
    p.status === BraveWallet.TransactionStatus.Approved ||
    p.status === BraveWallet.TransactionStatus.Unapproved ||
    p.status === BraveWallet.TransactionStatus.Signed
      ? 0.4
      : 1};
  background-color: ${(p) =>
    p.status === BraveWallet.TransactionStatus.Confirmed ||
    p.status === BraveWallet.TransactionStatus.Approved
      ? p.theme.color.successBorder
      : p.status === BraveWallet.TransactionStatus.Rejected ||
        p.status === BraveWallet.TransactionStatus.Error ||
        p.status === BraveWallet.TransactionStatus.Dropped
      ? p.theme.color.errorBorder
      : p.status === BraveWallet.TransactionStatus.Unapproved
      ? p.theme.color.interactive08
      : p.theme.color.warningIcon};
  margin-right: 6px;
`

// Buttons
export const WalletButton = styled.button`
  ${walletButtonFocusMixin}
`

export const WalletLink = styled(Link)`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 13px;
  line-height: 20px;
  text-align: center;
  color: ${(p) => p.theme.color.interactive05};
  background: none;
  border: none;

  text-decoration: none;

  &:hover {
    cursor: pointer;
  }

  &:active {
    opacity: 0.5;
  }
`

export const WalletButtonLink = styled(Link)`
  ${walletButtonFocusMixin}
`

export const ToggleVisibilityButton = styled.button<{
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
  -webkit-mask-image: url(${(p) => (p.isVisible ? EyeOnIcon : EyeOffIcon)});
  mask-image: url(${(p) => (p.isVisible ? EyeOnIcon : EyeOffIcon)});
  mask-size: contain;
  mask-position: center;
  mask-repeat: no-repeat;
  &:focus-visible {
    outline: auto;
    outline-style: solid;
    outline-color: ${(p) => p.theme.palette.blurple300};
    outline-width: 2px;
  }
`

export const CopyButton = styled(WalletButton)<{
  iconColor?: keyof IThemeProps['color']
}>`
  cursor: pointer;
  outline: none;
  border: none;
  mask-image: url(${ClipboardSvg});
  mask-position: center;
  mask-repeat: no-repeat;
  mask-size: 14px;
  background-color: ${(p) => p.theme.color[p?.iconColor ?? 'text01']};
  height: 14px;
  width: 14px;
`

export const DownloadButton = styled(WalletButton)`
  cursor: pointer;
  outline: none;
  border: none;
  mask-image: url(${DownloadSvg});
  mask-position: center;
  mask-repeat: no-repeat;
  mask-size: 14px;
  background-color: ${(p) => p.theme.color.text01};
  height: 14px;
  width: 14px;
`

export const SellButtonRow = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  width: 54px;
  height: 100%;
`

export const SellButton = styled(WalletButton)`
  display: flex;
  flex-direction: row;
  justify-content: center;
  align-items: center;
  padding: 6px 12px;
  cursor: pointer;
  outline: none;
  border-radius: 40px;
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 600;
  font-size: 12px;
  background-color: ${(p) => p.theme.palette.blurple500};
  color: ${(p) => p.theme.palette.white};
  border: none;
`

// Icons
export interface AssetIconProps {
  icon?: string
}

export const AssetIconFactory = styled.img.attrs<AssetIconProps>((props) => ({
  src: stripERC20TokenImageURL(props.icon)
    ? props.icon
    : // Display theme background (using a transparent image) if no icon to
      // render.
      transparent40x40Image,

  // Defer loading the image until it reaches a calculated distance from the
  // viewport, as defined by the browser.
  //
  // Ref: https://web.dev/browser-level-image-lazy-loading
  loading: 'lazy'
}))

// Construct styled-component using JS object instead of string, for editor
// support with custom AssetIconFactory.
//
// Ref: https://styled-components.com/docs/advanced#style-objects
export const SmallAssetIcon = AssetIconFactory<AssetIconProps>({
  width: '24px',
  height: 'auto'
})
export const MediumAssetIcon = AssetIconFactory<AssetIconProps>({
  width: '40px',
  height: 'auto'
})
export const LargeAssetIcon = AssetIconFactory<AssetIconProps>({
  width: '60px',
  height: 'auto'
})

export const GreenCheckmark = styled.div`
  display: inline-block;
  width: 10px;
  height: 10px;
  margin-right: 4px;
  background-color: ${(p) => p.theme.color.successBorder};
  mask: url(${CheckmarkSvg}) no-repeat 50% 50%;
  mask-size: contain;
  vertical-align: middle;
`

export const WarningTriangleFilledIcon = styled.div<{
  color?: keyof IThemeProps['color']
}>`
  mask-size: 100%;
  background-color: ${(p) =>
    p?.color ? p.theme.color[p.color] : leo.color.systemfeedback.warningIcon};
  -webkit-mask-image: url(${WarningTriangleFilled});
  mask-repeat: no-repeat;
  mask-image: url(${WarningTriangleFilled});
  @media (prefers-color-scheme: dark) {
    color: ${(p) => p.theme.palette.blurple300};
  }
`

export const WarningCircleFilledIcon = styled.div<{
  color?: keyof IThemeProps['color']
}>`
  opacity: 50%;
  mask-size: contain;
  mask-position: center;
  mask-repeat: no-repeat;
  background-color: ${(p) =>
    p?.color ? p.theme.color[p.color] : leo.color.systemfeedback.errorIcon};
  -webkit-mask-image: url(${WarningCircleFilled});
  mask-image: url(${WarningCircleFilled});
`

export const CloseIcon = styled.div`
  width: 20px;
  height: 20px;
  background-color: ${(p) => p.theme.color.text02};
  -webkit-mask-image: url(${CloseSvg});
  mask-image: url(${CloseSvg});
  mask-size: 20px;
  mask-position: center center;
  display: inline-block;
`

export const ErrorXIcon = styled.div`
  width: 12px;
  height: 12px;
  background-color: ${(p) => p.theme.color.errorIcon};
  -webkit-mask-image: url(${CloseSvg});
  mask-image: url(${CloseSvg});
  mask-size: 12px;
  mask-position: center center;
  margin-right: 10px;
  margin-bottom: 10px;
  display: inline-block;
`

export const LoadingIcon = styled(LoaderIcon as FC<{}>)<{
  size: string
  color: keyof IThemeProps['color']
  opacity: number
}>`
  color: ${(p) => p.theme.color[p.color]};
  height: ${(p) => p.size};
  width: ${(p) => p.size};
  opacity: ${(p) => p.opacity};
`

export const CheckIcon = styled.div<{
  color?: IThemeProps['color']
}>`
  width: 100%;
  height: 100%;
  background-color: ${(p) => p?.color || p.theme.color.text};
  -webkit-mask-image: url(${CheckIconSvg});
  mask-image: url(${CheckIconSvg});
  mask-repeat: no-repeat;
  mask-size: 12px;
  mask-position: center center;
  display: inline-block;
`

export const SwitchAccountIcon = styled.div`
  cursor: pointer;
  display: block;
  width: 14px;
  height: 14px;
  background: url(${SwitchDown});
  margin-left: 6px;
  margin-right: 6px;
`

export const LaunchIcon = styled(Icon).attrs({ name: 'launch' })`
  --leo-icon-size: 14px;
  --leo-icon-color: ${leo.color.icon.interactive};
  margin-bottom: 1px;
`

// Asset Icon containers
export const IconsWrapper = styled.div<{
  marginRight?: string
}>`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  position: relative;
  margin-right: ${(p) => p.marginRight || '6px'};
`

export const NetworkIconWrapper = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: row;
  position: absolute;
  bottom: 0px;
  right: 4px;
  background-color: ${(p) => p.theme.color.background02};
  border-radius: 100%;
  padding: 2px;
`

// Graphics
export const WalletWelcomeGraphic = styled.div<{
  scale?: CSSProperties['scale']
}>`
  width: 350px;
  height: 264px;
  background: url(${BraveWalletWithCoins});
  background-repeat: no-repeat;
  transform: scale(${(p) => p.scale ?? 1});
`

// spacers
export const VerticalSpace = styled.div<{ space: string }>`
  display: block;
  height: ${(p) => p.space};
  width: 100%;
`

export const HorizontalSpace = styled.div<{ space: string }>`
  min-height: 1px;
  width: ${(p) => p.space};
`

// Forms
export const InputLabelText = styled.label`
  font-family: Poppins;
  font-style: normal;
  display: block;
  margin-bottom: 8px;
  color: ${(p) => p.theme.color.text03};
  text-align: left;
  width: 100%;
`

export const VerticalDivider = styled.div<{ margin?: string }>`
  height: 1px;
  width: 100%;
  background-color: ${leo.color.divider.subtle};
  margin: ${(p) => p.margin || 0};
`

export const BraveRewardsIndicator = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  font-family: Poppins;
  font-size: 10px;
  line-height: 15px;
  font-weight: 500;
  color: ${leo.color.text.primary};
  padding: 2px 6px;
  border: 1px solid ${leo.color.divider.subtle};
  border-radius: 4px;
`

export const LeoSquaredButton = styled(Button)`
  --leo-button-radius: 12px;
`
