/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled, { css } from '../../../theme'
import Heading, { HeadingProps } from '../../../components/text/heading'

// TODO: consider exporting props to the main export file
export const NewTabHeading = styled<HeadingProps & React.HTMLProps<HTMLHeadingElement>, any>(Heading)`
  ${p => p.level === 2 && css`
    display: flex;
    white-space: nowrap;
    margin: 6px 0 10px;
    font-family: ${p => p.theme.fontFamily.heading};
    font-size: 20px;
    font-weight: 400;
    color: rgba(255, 255, 255, 1);
    letter-spacing: -0.2px;
  `}

  ${p => p.level === 3 && css`
    line-height: 1.75;
    font-size: 16px;
    color: rgba(255,255,255,0.8);
    font-family: ${p => p.theme.fontFamily.heading};
  `}
`

export const Paragraph = styled<React.HTMLProps<HTMLParagraphElement>, 'p'>('p')`
  display: flex;
  align-items: center;
  margin: 0 0 10px;
  margin: 0;
  line-height: 1.75;
  font-size: 16px;
  color: rgba(255,255,255,0.8);
  -webkit-font-smoothing: antialiased;
  font-family: ${p => p.theme.fontFamily.body};
`

export const LinkText = styled<React.HTMLProps<HTMLAnchorElement>, 'a'>('a')`
  line-height: 1.75;
  font-size: 16px;
  color: #FF6000;
  font-family: inherit;
  display: inline-block;
  cursor: pointer;
  text-decoration: underline;
`

export const EmphasizedText = styled<React.HTMLProps<HTMLSpanElement>, 'span'>('span')`
  font-family: ${p => p.theme.fontFamily.heading};
  font-size: 20px;
  font-weight: 600;
  color: rgba(255, 255, 255, 1);
  letter-spacing: -0.2px;
  font-style: normal;
`

export const LabelledText = styled<React.HTMLProps<HTMLSpanElement>, 'span'>('span')`
  padding: 3px 8px;
  border-radius: 12px;
  text-transform: uppercase;
  margin-left: 10px;
  letter-spacing: 0.5px;
  background: #0795fa;
  color: white;
  align-self: center;
  font-weight: 600;
  font-size: 11px;
  font-family: ${p => p.theme.fontFamily.heading};
`

export const SmallText = styled<React.HTMLProps<HTMLSpanElement>, 'span'>('span')`
  margin: 0;
  line-height: 1.5;
  font-size: 13px;
  color: rgba(255, 255, 255, .5);
  font-family: ${p => p.theme.fontFamily.body};
`
