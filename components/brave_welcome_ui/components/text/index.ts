/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

export const Title = styled.h1`
  font-family: ${p => p.theme.fontFamily.body};
  color: ${p => p.theme.color.text};
  font-size: 34px;
  margin: 24px 0 0;
  text-align: center;
  line-height: 44px;
  font-weight: 600;
`

export const Paragraph = styled.p`
  color: ${p => p.theme.color.text};
  font-family: ${p => p.theme.fontFamily.body};
  display: block;
  -webkit-font-smoothing: antialiased;
  font-size: 18px;
  opacity: 0.65;
  line-height: 36px;
  text-align: center;
  margin: 12px 0 32px;
  a {
    box-sizing: border-box;
    color: ${p => p.theme.color.brandBrave};
    border: 0;
    padding: 0;
    background: transparent;
    cursor: pointer;
    font-size: inherit;
    display: inline-block;
    text-align: left;
    width: fit-content;
    &:hover {
      text-decoration: underline;
    }
    &:focus {
      outline-color: ${p => p.theme.color.brandBrave};
      outline-width: 2px;
    }
    &:active {
      outline: none;
    }
  }
`

export const TermsOfService = styled(Paragraph)`
  font-size: 13px;
  line-height: 20px;
  opacity: .8;

  /* Collapse bottom margin of preceding Paragraph */
  ${Paragraph} + & {
    margin-top: -19px;
  }
`
