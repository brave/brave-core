// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react'
import styled from 'styled-components'
import { font, gradient, icon, radius, spacing } from '@brave/leo/tokens/css/variables'
import Button from '@brave/leo/react/button'
import Icon from '@brave/leo/react/icon'
import Label from '@brave/leo/react/label'
import InitialText from './InitialText'
import NoContent from './NoContent'
import { useRewriterContext } from '../Context'

const TitleText = styled.h2`
  font: ${font.heading.h3};
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

const FiltersContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: center;
  align-self: stretch;
  gap: ${spacing['2Xl']};

  padding: 0 ${spacing['2Xl']} ${spacing['2Xl']} ${spacing['2Xl']};
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
  const { undo, canUndo, redo, canRedo, erase, canErase, generate, close, openSettings } = useRewriterContext()
  return <Root>
    <TitleRow>
      <Icon name='product-brave-leo' />
      <TitleText>leo writer</TitleText>
      <Label color='blue'>PREMIUM</Label>
      <DialogControls>
        <Button fab kind='plain-faint' size='small' onClick={openSettings}>
          <Icon name='settings' />
        </Button>
        <Button fab kind='plain-faint' size='small' onClick={close}>
          <Icon name='close' />
        </Button>
      </DialogControls>
    </TitleRow>
    <FiltersContainer>
      <InitialText />
    </FiltersContainer>
    <NoContent />
    <ActionsRow>
      <Button fab kind="outline" isDisabled={!canUndo} onClick={undo}>
        <Icon name="arrow-undo" />
      </Button>
      <Button fab kind="outline" isDisabled={!canRedo} onClick={redo}>
        <Icon name="arrow-redo" />
      </Button>
      <Button fab kind="outline" isDisabled={!canErase} onClick={erase}>
        <Icon name="erase" />
      </Button>
      <FlexSpacer />
      <Button onClick={generate}>
        Generate
      </Button>
    </ActionsRow>
  </Root>
}
