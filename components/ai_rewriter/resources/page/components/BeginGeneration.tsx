// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { spacing } from "@brave/leo/tokens/css/variables";
import * as React from "react";
import styled from "styled-components";
import InputBox from '../../../../ai_chat/resources/page/components/input_box';
import ToolsButtonMenu from "../../../../ai_chat/resources/page/components/tools_button_menu";
import { useRewriterContext } from "../Context";
import InitialText from "./InitialText";
import NoContent from "./NoContent";

const FiltersContainer = styled.div`
  display: flex;
  flex-direction: column;
  align-items: stretch;
  align-self: stretch;
  gap: ${spacing['2Xl']};

  padding: 0 ${spacing['2Xl']} ${spacing['2Xl']} ${spacing['2Xl']};

  & > leo-buttonmenu {
    --leo-menu-max-height: 280px !important;
  }
`

export default function BeginGeneration() {
  const context = useRewriterContext()
  return <>
    <FiltersContainer>
      <InitialText />
      <ToolsButtonMenu isToolsMenuOpen={context.isToolsMenuOpen} setIsToolsMenuOpen={context.setIsToolsMenuOpen} actionList={context.actionList} handleActionTypeClick={context.handleActionTypeClick} inputText={context.instructionsText}>
        <InputBox
          context={{
            inputText: context.instructionsText,
            setInputText: context.setInstructionsText,
            isToolsMenuOpen: context.isToolsMenuOpen,
            setIsToolsMenuOpen: context.setIsToolsMenuOpen,
            resetSelectedActionType: context.resetSelectedActionType,
            selectedActionType: context.selectedActionType,
            submitInputTextToAPI: context.submitRewriteRequest,
            inputTextCharCountDisplay: context.inputTextCharCountDisplay,
            isCharLimitApproaching: context.isCharLimitApproaching,
            isCharLimitExceeded: context.isCharLimitExceeded,
            shouldDisableUserInput: context.isGenerating,
            isMobile: false,
            hasAcceptedAgreement: true
          }}
          onFocusInput={() => undefined}
          onBlurInput={() => undefined} />
      </ToolsButtonMenu>
    </FiltersContainer>
    <NoContent />
  </>
}
