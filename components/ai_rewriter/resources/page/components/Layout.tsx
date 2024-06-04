// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'
import { font, gradient, icon, radius, spacing } from '@brave/leo/tokens/css/variables'
import * as React from 'react'
import styled from 'styled-components'
import { useRewriterContext } from '../Context'
import BeginGeneration from './BeginGeneration'
import ModifyGeneration from './ModifyGeneration'

const TitleText = styled.h2`
  font: ${font.heading.h3};

  font-size: 16px;
  line-height: 16px;
`

const TitleRow = styled.div`
  display: flex;
  align-items: center;
  gap: ${spacing.m};

  padding: ${spacing['2Xl']};

  &> leo-icon {
    --leo-icon-color: ${gradient.iconsActive};
  }
`

const DialogControls = styled.div`
  --leo-icon-size: ${icon.m};

  position: absolute;
  top: ${spacing.xl};
  right: ${spacing.xl};

  display: flex;
  gap: ${spacing.xl};
`

const ActionsRow = styled.div`
  display: flex;
  align-items: center;
  align-self: stretch;
  gap: ${spacing.m};
  padding: ${spacing['2Xl']};

  &> leo-button {
    flex-grow: 0;
  }
`

const FlexSpacer = styled.div`
  flex-grow: 1;
`

const Root = styled.div`
  display: flex;
  flex-direction: column;
  justify-content: center;
  align-content: center;

  border-radius: ${radius.xl};
`

export default function Layout() {
  const context = useRewriterContext()
  return <Root>
    <TitleRow>
      <Icon name='product-brave-leo' />
      <TitleText>leo writer</TitleText>
      <Label color='blue'>PREMIUM</Label>
      <DialogControls>
        <Button fab kind='plain-faint' size='small' onClick={context.openSettings}>
          <Icon name='settings' />
        </Button>
        <Button fab kind='plain-faint' size='small' onClick={context.close}>
          <Icon name='close' />
        </Button>
      </DialogControls>
    </TitleRow>
    {context.generatedText
      ? <ModifyGeneration />
      : <BeginGeneration />}
    <ActionsRow>
      <Button fab kind="outline" isDisabled={!context.canUndo} onClick={context.undo}>
        <Icon name="arrow-undo" />
      </Button>
      <Button fab kind="outline" isDisabled={!context.canRedo} onClick={context.redo}>
        <Icon name="arrow-redo" />
      </Button>
      <Button fab kind="outline" isDisabled={!context.canErase} onClick={context.erase}>
        <Icon name="erase" />
      </Button>
      <FlexSpacer />
      <Button
        isLoading={context.isGenerating}
        onClick={context.generatedText ? context.acceptGeneratedText : context.submitRewriteRequest}
        isDisabled={context.isGenerating || (!context.instructionsText && !context.selectedActionType)}>
        {context.isGenerating
          ? "Generating"
          : context.generatedText
            ? "Insert"
            : "Generate"}
      </Button>
    </ActionsRow>
  </Root>
}
