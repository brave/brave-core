/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import {
  StyledWrapper,
  StyledCloseIcon,
  StyledBackground,
  StyledContent,
  StyledTitle,
  StyledDisclaimer,
  StyleRight
} from './style'
import { CloseStrokeIcon } from 'brave-ui/components/icons'

export interface Props {
  title: string
  copy: JSX.Element
  imagePath: string
  disclaimer?: string | JSX.Element
  onDismissPromo: (event: React.MouseEvent<HTMLDivElement>) => void
  link: string
}

export default class SidebarPromo extends React.PureComponent<Props, {}> {
  render () {
    const {
      copy,
      title,
      imagePath,
      disclaimer,
      onDismissPromo,
      link
    } = this.props

    return (
      <StyledWrapper href={link} target={'_blank'}>
        <StyledBackground src={imagePath} />
        <StyleRight>
          <StyledCloseIcon onClick={onDismissPromo}>
            <CloseStrokeIcon />
          </StyledCloseIcon>
          <StyledContent>
            <StyledTitle>
              {title}
            </StyledTitle>
            {copy}
            {
              disclaimer
              ? <StyledDisclaimer>
                  {disclaimer}
                </StyledDisclaimer>
              : null
            }
          </StyledContent>
        </StyleRight>
      </StyledWrapper>
    )
  }
}
