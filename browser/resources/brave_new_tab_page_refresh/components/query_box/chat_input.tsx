/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useAIChat } from '../../../../../components/ai_chat/resources/page/state/ai_chat_context'
import { useConversation } from '../../../../../components/ai_chat/resources/page/state/conversation_context'
import InputBox from '../../../../../components/ai_chat/resources/page/components/input_box'
import ToolsMenu from '../../../../../components/ai_chat/resources/page/components/filter_menu/tools_menu'
import TabsMenu from '../../../../../components/ai_chat/resources/page/components/filter_menu/tabs_menu'
import { useExtractedQuery } from '../../../../../components/ai_chat/resources/page/components/filter_menu/query'
import { openLink } from '../common/link'

import { style } from './chat_input.style'

interface Props {
  renderInputControls?: () => React.ReactNode
}

export function ChatInput(props: Props) {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()
  const { conversationUuid, conversationHistory } = conversationContext

  const extractedQuery = useExtractedQuery(conversationContext.inputText, {
    onlyAtStart: true,
    triggerCharacter: '/'
  })

  React.useEffect(() => {
    if (conversationUuid && conversationHistory.length > 0) {
      openLink('chrome://leo-ai/' + encodeURIComponent(conversationUuid))
    }
  }, [conversationUuid, conversationHistory.length])

  return (
    <div data-css-scope={style.scope}>
      <InputBox
        conversationStarted={false}
        context={{ ...conversationContext, ...aiChatContext }}
        maybeShowSoftKeyboard={() => false}
        renderInputControls={props.renderInputControls}
      />
      <div className='chat-tools'>
        <TabsMenu placement='bottom' />
        <ToolsMenu
          isOpen={conversationContext.isToolsMenuOpen}
          setIsOpen={conversationContext.setIsToolsMenuOpen}
          query={extractedQuery}
          categories={aiChatContext.actionList}
          handleClick={conversationContext.handleActionTypeClick}
          placement='bottom'
        />
      </div>
    </div>
  )
}
