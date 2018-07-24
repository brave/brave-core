/* This Source Code Form is subject to the terms of the Mozilla Public
 * License. v. 2.0. If a copy of the MPL was not distributed with this file.
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

import styled from 'styled-components'

const StyledInfoCard = styled.div`
  height: 290px;
  border-radius: 4px;
  text-align:center;
  padding: 15px;
  background: #fff;
  max-width: 275px;
  margin: 0 auto;
  box-shadow: 0 0 10px 0 rgba(99,105,110,0.12);
` as any

const StyledTitle = styled.strong`
  color: #222326;
  font-size: 18px;
  font-weight: 500;
  line-height: 28px;
  font-family: Poppins, sans-serif;
  letter-spacing: 0.16px;
` as any

const StyledDesc = styled.p`
  color: #484B4E;
  font-size: 16px;
  font-weight: 300;
  line-height: 22px;
  font-family: Muli, sans-serif;
  letter-spacing: 0.16px;
` as any

const StyledFigure = styled.figure`
  box-sizing: border-box;
  display: block;
  max-width: 100%;
  margin: 0;
` as any

const StyledImage = styled.img`
  max-width: 80px;
  min-height: 80px;
  margin: 10px auto 20px auto;
` as any

export {
  StyledTitle,
  StyledDesc,
  StyledInfoCard,
  StyledImage,
  StyledFigure
}
