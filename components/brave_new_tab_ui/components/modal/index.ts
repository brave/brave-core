/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import { ComponentType } from 'react'
import styled from '../../../theme'
import DefaultModal, { Props } from '../../../components/popupModals/modal'
import Heading from '../../../components/text/heading'

export const Modal = styled(DefaultModal as ComponentType<Props>)`
  height: 100%;
`

export const LimitedBounds = styled<{}, 'article'>('article')`
  overflow: auto;
  height: 500px;
`

export const HeadingText = styled(Heading)`
  font-size: 18px;
  font-weight: normal;
`

export const Paragraph = styled<{}, 'p'>('p')`
  font-size: 14px;
  font-family: ${p => p.theme.fontFamily.heading};
  letter-spacing: 0.14px;
`

export const Footer = styled<{}, 'footer'>('footer')`
  display: flex;
  justify-content: flex-end;
`
