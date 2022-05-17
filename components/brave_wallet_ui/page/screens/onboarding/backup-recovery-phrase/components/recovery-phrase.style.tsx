// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

import FrostedGlassRecoveryPhrase from '../images/frosted-glass-recovery-phrase.png'

export const RecoveryPhraseContainer = styled.div`
  display: flex;
  align-items: flex-start;
  justify-content: space-between;
  flex-direction: row;
  flex-wrap: wrap;
  padding-left: 20px;
  padding-right: 20px;
  padding-top: 16px;
`

export const RecoveryBubble = styled.div`
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  background-color: ${(p) => p.theme.color.background01};
  padding: 5px 0px;
  border-radius: 4px;
  flex-basis: 100px;
  margin-bottom: 6px;
`

export const RecoveryBubbleText = styled.span`
  font-family: Poppins;
  font-size: 14px;
  line-height: 22px;
  font-weight: 600;
  color: ${(p) => p.theme.color.text01};
`

export const FrostedGlass = styled.div`
  position: absolute;
  top: 0;
  bottom: 0; 
  left: 0;
  right: 0;
  background-image: url(${FrostedGlassRecoveryPhrase});
  border-radius: 4px;
  display: flex;
  flex-direction: column;
  align-items: center;
  justify-content: center;
  flex: 1;
`

export const HiddenPhraseContainer = styled.div`
  position: relative;
  top: 0;
  bottom: 0;
  left: 0;
  right: 0;
  padding: 0;
  height: 100%;
  min-height: 100%;
`
