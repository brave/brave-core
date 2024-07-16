// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import { ActionGroup, ActionType } from 'gen/brave/components/ai_chat/core/common/mojom/ai_chat.mojom.m';
import { AIRewriterPageCallbackRouter, AIRewriterPageHandler, AIRewriterPageHandlerRemote } from 'gen/brave/components/ai_rewriter/common/mojom/ai_rewriter.mojom.m';
import * as React from 'react';
import { CharCountContext, defaultCharCountContext, getFirstValidAction, useActionMenu, useCharCountInfo } from '../../../ai_chat/resources/page/state/conversation_context';

interface Context extends CharCountContext {
  initialText: string;
  setInitialText: (next: string) => void

  instructionsText: string,
  setInstructionsText: (instructions: string) => void

  isToolsMenuOpen: boolean,
  setIsToolsMenuOpen: (open: boolean) => void

  submitRewriteRequest: () => void,
  generatedText?: string,

  resetSelectedActionType: () => void,
  selectedActionType: ActionType | undefined,
  handleActionTypeClick: (actionType: ActionType) => void,
  actionList: ActionGroup[]

  isGenerating: boolean,

  undo: () => void,
  canUndo: boolean,

  redo: () => void,
  canRedo: boolean,

  erase: () => void,
  canErase: boolean,

  close: () => void,
  openSettings: () => void,

  acceptGeneratedText: () => void
}

const RewriterContext = React.createContext<Context>({
  initialText: '',
  setInitialText: () => { },

  instructionsText: '',
  setInstructionsText: () => { },

  selectedActionType: undefined,
  resetSelectedActionType: () => { },
  handleActionTypeClick: () => { },
  actionList: [],

  isToolsMenuOpen: false,
  setIsToolsMenuOpen: () => { },

  submitRewriteRequest: () => { },

  generatedText: undefined,
  isGenerating: false,

  undo: () => { },
  canUndo: false,

  redo: () => { },
  canRedo: false,

  erase: () => { },
  canErase: false,

  close: () => { },
  openSettings: () => { },

  acceptGeneratedText: () => { },

  ...defaultCharCountContext,
})

let pageHandler: AIRewriterPageHandlerRemote
let callbackRouter: AIRewriterPageCallbackRouter
export function getRewriterPageHandler() {
  if (!pageHandler) {
    pageHandler = AIRewriterPageHandler.getRemote()
    callbackRouter = new AIRewriterPageCallbackRouter()
    pageHandler.setPage(callbackRouter.$.bindNewPipeAndPassRemote());
  }
  return pageHandler;
}

export function getCallbackRouter() {
  getRewriterPageHandler()
  return callbackRouter
}

export const useRewriterContext = () => React.useContext(RewriterContext)

export default function Context(props: React.PropsWithChildren) {
  const [initialText, setInitialText] = React.useState('')
  const [instructionsText, setInstructionsText] = React.useState('')
  const [isToolsMenuOpen, setIsToolsMenuOpen] = React.useState(false)
  const [actionType, setActionType] = React.useState<ActionType>()
  const [generatedText, setGeneratedText] = React.useState<string>()
  const generatedTextRef = React.useRef<string | undefined>(generatedText)
  React.useEffect(() => {
    generatedTextRef.current = generatedText
  }, [generatedText])

  const [forwardHistory, setForwardHistory] = React.useState<string[]>([])
  const [backHistory, setBackHistory] = React.useState<string[]>([])
  const [isGenerating, setIsGenerating] = React.useState(false)

  const actionList = useActionMenu(instructionsText, () => getRewriterPageHandler().getActionMenuList().then(({ actionList }) => actionList))
  const charCountContext = useCharCountInfo(instructionsText)

  React.useEffect(() => {
    getRewriterPageHandler()
      .getInitialText()
      .then(({ initialText }) => setInitialText(initialText))

    const callbackRouter = getCallbackRouter()
    callbackRouter.onUpdatedGeneratedText.addListener(setGeneratedText)
    return () => {
      callbackRouter.onUpdatedGeneratedText.removeListener(setGeneratedText)
    }
  }, [])

  React.useEffect(() => {
    if (instructionsText.startsWith('/'))
      setIsToolsMenuOpen(true)
  }, [instructionsText])

  const context = React.useMemo<Context>(() => ({
    initialText,
    setInitialText,

    instructionsText,
    setInstructionsText,

    isToolsMenuOpen: isToolsMenuOpen,
    setIsToolsMenuOpen: setIsToolsMenuOpen,

    submitRewriteRequest: () => {
      // Don't let multiple generation requests run simultaneously
      if (isGenerating) return

      const rewriteText = generatedText || initialText
      if (!rewriteText) return
      if (charCountContext.isCharLimitExceeded) return
      if (isToolsMenuOpen && instructionsText.startsWith('/')) {
        setActionType(getFirstValidAction(actionList))
        setInstructionsText('')
        setIsToolsMenuOpen(false)
        return
      }

      setIsGenerating(true)
      getRewriterPageHandler()
        // If we have some generated text, generate from that instead of the initial text.
        .rewriteText(generatedText || initialText, actionType ?? ActionType.UNSPECIFIED, instructionsText)
        .then(() => {
          // Reset Action/Instructions
          setActionType(undefined)
          setInstructionsText('')

          // Don't update history if the text didn't change.
          if (generatedTextRef.current === generatedText) {
            return
          }

          // Update history
          // TODO: include action & instructions in history
          setForwardHistory([])
          setBackHistory(h => [...h, generatedText!])
        })
        .finally(() => setIsGenerating(false))
    },
    generatedText,
    isGenerating,

    selectedActionType: actionType,
    resetSelectedActionType: () => setActionType(undefined),
    handleActionTypeClick: (action) => {
      if (instructionsText.startsWith('/'))
        setInstructionsText('')

      setActionType(action)
      setIsToolsMenuOpen(false)
    },
    actionList,

    undo: () => {
      if (backHistory.length === 0) return
      const copy = [...backHistory]
      const last = copy.at(-1)
      copy.length -= 1

      setBackHistory(copy)
      if (generatedText)
        setForwardHistory(h => [...h, generatedText])
      setGeneratedText(last)
    },
    canUndo: backHistory.length > 0 && !isGenerating,

    redo: () => {
      if (forwardHistory.length === 0) return

      const copy = [...forwardHistory]
      const last = copy.at(-1)
      copy.length -= 1

      setForwardHistory(copy)
      setBackHistory(h => [...h, generatedText!])
      setGeneratedText(last)
    },
    canRedo: forwardHistory.length > 0 && !isGenerating,

    erase: () => {
      if (!generatedText) return
      setBackHistory(h => [...h, generatedText])
      setGeneratedText(undefined)
    },
    canErase: generatedText !== undefined && !isGenerating,

    close: () => getRewriterPageHandler().close(),

    openSettings: () => getRewriterPageHandler().openSettings(),

    acceptGeneratedText: () => {
      if (!generatedText) return

      setIsGenerating(true)
      getRewriterPageHandler().insertTextAndClose(generatedText)
        .finally(() => setIsGenerating(false))
    },

    ...charCountContext
  }), [initialText, instructionsText, actionType, actionList, generatedText, isGenerating, forwardHistory, charCountContext.inputTextCharCountDisplay, charCountContext.isCharLimitApproaching, charCountContext.isCharLimitExceeded, backHistory, actionList, isToolsMenuOpen])

  return <RewriterContext.Provider value={context}>
    {props.children}
  </RewriterContext.Provider>
}
