// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import styled from 'styled-components'

// images
import EyeOffSvg from '../../../assets/svg-icons/eye-off-icon.svg'
import FrostedGlassRecoveryPhrase from './images/frosted-glass-recovery-phrase.png'

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

export const RecoveryBubble = styled.div<{
  verificationModeEnabled?: boolean
  selected?: boolean
}>`
  position: ${(p) => p.verificationModeEnabled ? 'relative' : 'unset'};
  display: flex;
  align-items: center;
  justify-content: center;
  flex-direction: column;
  padding: 5px 0px;
  border-radius: 4px;
  flex-basis: 100px;
  margin-bottom: ${(p) => p.verificationModeEnabled
    ? 24
    : 16
  }px;
  cursor: ${(p) => p.verificationModeEnabled
    ? 'pointer'
    : 'unset'
  };

  font-family: Poppins;
  font-size: 14px;
  line-height: 22px;
  font-weight: 600;


  color: ${(p) => p.theme.color.text01};
  background-color: ${(p) => p.selected
    ? p.theme.palette.blurple200
    : p.theme.color.background01
  };
  
  @media (prefers-color-scheme: dark) {
    color: ${(p) => p.selected
      ? p.theme.palette.black
      : p.theme.color.text01
    };
    background-color: ${(p) => p.selected
    ? p.theme.palette.blurple200
    : p.theme.color.background01
  };
  }

`

export const RecoveryBubbleBadge = styled.p`
  position: absolute;
  top: -15px;
  left: -8px;
  color: ${(p) => p.theme.palette.white};
  background-color: ${(p) => p.theme.color.brandBat};
  width: 40px;
  border-radius: 4px;
  font-family: 'Poppins';
  font-style: normal;
  font-weight: 500;
  font-size: 10px;
  line-height: 15px;
  display: flex;
  align-items: center;
  justify-content: center;
  text-align: center;
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
  cursor: pointer;

  & p {
    font-family: 'Poppins';
    font-style: normal;
    font-weight: 600;
    font-size: 13px;
    line-height: 20px;
    text-align: center;
    margin-top: 10px;
    color: ${(p) => p.theme.color.background01};
  }
`

export const EyeOffIcon = styled.div`
  height: 24px;
  width: 24px;
  mask-image: url(${EyeOffSvg});
  mask-repeat: no-repeat;
  mask-position: center;
  mask-size: 24px;
  background-color: ${(p) => p.theme.color.background01};
`
