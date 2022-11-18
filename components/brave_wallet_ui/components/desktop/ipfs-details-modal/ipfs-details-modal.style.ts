// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import styled from 'styled-components'

export const IpfsDetailsModalWrapper = styled.div`
  position: relative;
  display: flex;
  align-items: center;
  justify-content: center;
  width: 412px;
`

export const ContentWrapper = styled.div`
  display: flex;
  display: flex;
  align-items: flex-start;
  justify-content: center;
  width: 400px;
  flex-direction: column;
  padding: 24px 16px 34px;
  background-color: ${p => p.theme.palette.white};
  box-shadow: 0px 0px 24px rgba(99, 105, 110, 0.36);
  border-radius: 8px;
`

export const TitleText = styled.p`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  margin: 0;
  padding: 0;
  color: ${p => p.theme.color.text03};
  display: flex;
  justify-self: left;
`

export const IpfsDetailsText = styled.p`
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 14px;
  line-height: 20px;
  margin: 4px 0 0 0;
  padding: 0;
  color: ${p => p.theme.palette.text01};
  display: flex;
  justify-self: left;
  width: 100%;
`

const Section = styled.section`
  display: flex;
  flex-direction: column;
  background-color: #F8F9FA;
  border-radius: 8px;
`

export const PeerSection = styled(Section)`
  margin: 24px 0;
  padding: 8px;
`

export const SectionRow = styled.div`
  display: grid;
  grid-template-columns: 42px auto;
  column-gap: 8px;
`

export const SectionValue = styled.p`
  margin: 0;
  padding: 0;
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 400;
  font-size: 12px;
  line-height: 18px;
  white-space: wrap;
  overflow-wrap: break-word;
  width: 287px;
  color: ${p => p.theme.color.text02};
  margin-bottom: 8px;
  text-align: left;
`

export const SectionValueHighlighted = styled(SectionValue)`
  color: ${p => p.theme.color.interactive06};
`

export const NetworkSection = styled(Section)`
  width: 100%;
  margin-top: 16px;
  margin-bottom: 26px;
  padding: 16px;
  justify-content: flex-start;
`
