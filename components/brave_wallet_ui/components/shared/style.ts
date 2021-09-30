import styled from 'styled-components'

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
