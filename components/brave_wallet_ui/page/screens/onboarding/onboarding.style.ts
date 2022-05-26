// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// images
import WelcomeBackgroundSVG from './images/onboarding-welcome-background.svg'

// Layout
export const OnboardingWrapper = styled.div`
  width: 100%;
  height: 100%;
  padding: 80px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  overflow-x: hidden;
  background-repeat: no-repeat;
  background-image: url(${WelcomeBackgroundSVG});
  background-position: 50% 90%;
  background-size: 88%;
`

export const NextButtonRow = styled.div`
  display: flex;
  flex-direction: row;
  align-self: center;
  align-items: center;
  justify-content: center;
  width: 100px;
  margin-bottom: 28px;
`

export const MainWrapper = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  width: 100%;
`

export const StyledWrapper = styled.div`
  width: 376px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
`

export const TitleAndDescriptionContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  justify-content: flex-start;
  margin-bottom: 24px;
`

// Text
export const Title = styled.p`
  font-family: Poppins;
  font-size: 20px;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
  letter-spacing: 0.02em;
  margin-bottom: 16px;
  text-align: left;
`

export const Description = styled.p<{ textAlign?: 'right' | 'center' }>`
  display: flex;
  flex-direction: column;
  align-items: flex-start;
  font-family: Poppins;
  font-size: 14px;
  line-height: 20px;
  font-weight: 300;
  color: ${(p) => p.theme.color.text02};
  text-align: ${(p) => p?.textAlign || 'left'};

  & > * > b {
    font-family: 'Poppins';
    font-style: normal;
    font-weight: 600;
    font-size: 14px;
    line-height: 20px;
    letter-spacing: 0.01em;
  }
`

export const VerticalSpace = styled.div<{ space: string }>`
  display: block;
  height: ${(p) => p.space};
  width: 100%;
`

// Phrase card
export const PhraseCard = styled.div`
  display: flex;
  flex: 1;
  flex-direction: column;
  width: 376px;
  align-items: center;
`

export const PhraseCardTopRow = styled.div`
  display: flex;
  flex-direction: row;
  width: 375px;
  height: 40px;
  align-items: center;
  justify-content: flex-end;
  padding: 14px 8px;
`

export const PhraseCardBody = styled.div`
  flex: 1;
  width: 100%;
  border-style: solid;
  border-width: 1px;
  border-color: ${(p) => p.theme.color.divider01};
  border-radius: 4px;
`

export const PhraseCardBottomRow = styled(PhraseCardTopRow)<{
  centered?: boolean
}>`
  justify-content: ${(p) => p.centered ? 'center' : 'flex-start'};
  height: 40px;
  gap: 14px;
`
