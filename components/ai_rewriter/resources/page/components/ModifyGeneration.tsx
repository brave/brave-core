// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from "react";
import { useRewriterContext } from "../Context";
import styled from "styled-components";
import { color, effect, font, radius, spacing } from "@brave/leo/tokens/css/variables";
import Input from "@brave/leo/react/input";
import Icon from "@brave/leo/react/icon";
import Button from "@brave/leo/react/button";
import Flex from '$web-common/Flex'

const Content = styled.div`
  display: flex;
  padding: 0 ${spacing.m};
  flex-direction: column;
  justify-content: center;
  align-items: center;
  align-self: stretch;
`

const Response = styled.div`
  border-radius: ${radius.m};
  border: 1px solid ${color.divider.subtle};
  background: ${color.container.background};
  box-shadow: ${effect.elevation[1]};
  align-self: stretch;
  overflow: hidden;
`

const GeneratedText = styled.div`
  font: ${font.large.regular};
  padding: ${spacing["2Xl"]};

  height: 282px;
  overflow-y: auto;
`

const Bottom = styled.div`
  display: flex;
  padding: 0 ${spacing.m} ${spacing.m} ${spacing.m};
  justify-content: flex-end;
  align-items: center;
  gap: ${spacing.xl};
  justify-content: stretch;

  & > * {
    flex: 1;
  }
`

export default function ModifyGeneration() {
  const context = useRewriterContext()
  return <Content>
    <Response>
      <GeneratedText>
        {context.generatedText}
      </GeneratedText>
      <Bottom>
        <Input onKeyDown={e => {
          const event = e.innerEvent as any as KeyboardEvent
          if (event.key === 'Enter' && !event.shiftKey) {
            event.preventDefault()
            context.submitRewriteRequest()
          }
        }} disabled={context.isGenerating} placeholder="Tell Leo how to improve this text" value={context.instructionsText} onInput={e => context.setInstructionsText(e.value)}>
          <Flex slot="right-icon">
            <Button fab onClick={context.submitRewriteRequest} kind='plain-faint' isDisabled={!context.instructionsText || context.isGenerating}>
              <Icon name="send" />
            </Button>
          </Flex>
        </Input>
      </Bottom>
    </Response>
  </Content>
}
