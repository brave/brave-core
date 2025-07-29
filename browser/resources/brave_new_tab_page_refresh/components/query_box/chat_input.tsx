/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import Dialog from '@brave/leo/react/dialog'

import { useAIChat } from '../../../../../components/ai_chat/resources/page/state/ai_chat_context'
import { useConversation } from '../../../../../components/ai_chat/resources/page/state/conversation_context'
import InputBox from '../../../../../components/ai_chat/resources/page/components/input_box'
import { stringifyContent } from '../../../../../components/ai_chat/resources/page/components/input_box/editable_content'
import ToolsMenu, {
  getIsSkill,
} from '../../../../../components/ai_chat/resources/page/components/filter_menu/tools_menu'
import AttachmentsMenu from '../../../../../components/ai_chat/resources/page/components/filter_menu/attachments_menu'
import Attachments from '../../../../../components/ai_chat/resources/page/components/attachments'
import SkillModal from '../../../../../components/ai_chat/resources/page/components/skill_modal/skill_modal'
import { useExtractedQuery } from '../../../../../components/ai_chat/resources/page/components/filter_menu/query'
import { openLink } from '../common/link'

import { style } from './chat_input.style'

interface Props {
  renderInputToggle?: () => React.ReactNode
}

export function ChatInput(props: Props) {
  const aiChatContext = useAIChat()
  const conversationContext = useConversation()
  const { conversationUuid, conversationHistory } = conversationContext

  const extractedQuery = useExtractedQuery(
    stringifyContent(conversationContext.inputText),
    {
      onlyAtStart: true,
      triggerCharacter: '/',
    },
  )

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
        renderInputToggle={props.renderInputToggle}
      />
      <div className='chat-tools'>
        <AttachmentsMenu />
        <ToolsMenu
          isOpen={conversationContext.isToolsMenuOpen}
          isMobile={false}
          setIsOpen={conversationContext.setIsToolsMenuOpen}
          query={extractedQuery}
          categories={aiChatContext.actionList}
          handleClick={(entry) => {
            if (getIsSkill(entry)) {
              conversationContext.handleSkillClick(entry)
            } else {
              conversationContext.handleActionTypeClick(entry.details!.type)
            }
          }}
          handleEditClick={conversationContext.handleSkillEdit}
          handleNewSkillClick={() => {
            const inputText = stringifyContent(conversationContext.inputText)
            aiChatContext.setSkillDialog({
              id: '',
              shortcut: inputText.replace(/^\//, ''),
              prompt: '',
              model: '',
              createdTime: { internalValue: BigInt(0) },
              lastUsed: { internalValue: BigInt(0) },
            })
          }}
        />
        {conversationContext.attachmentsDialog && (
          <Dialog
            isOpen
            onClose={() => conversationContext.setAttachmentsDialog(null)}
          >
            <Attachments />
          </Dialog>
        )}
      </div>
      {aiChatContext.skillDialog && <SkillModal />}
    </div>
  )
}
