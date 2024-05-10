// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.
import * as React from 'react';
import { AIRewriterServiceRemote, AIRewriterService } from 'gen/brave/components/ai_rewriter/common/mojom/ai_rewriter.mojom.m'

interface Context {
  initialText: string;
  setInitialText: (next: string) => void

  undo: () => void,
  canUndo: boolean,

  redo: () => void,
  canRedo: boolean,

  erase: () => void,
  canErase: boolean,

  generate: () => void,

  close: () => void,
  openSettings: () => void,
}

const RewriterContext = React.createContext<Context>({
  initialText: '',
  setInitialText: () => { },

  undo: () => { },
  canUndo: false,

  redo: () => { },
  canRedo: false,

  erase: () => { },
  canErase: false,

  generate: () => { },

  close: () => { },
  openSettings: () => { },
})

let rewriterService: AIRewriterServiceRemote
export function getRewriterService() {
  if (!rewriterService) {
    rewriterService = AIRewriterService.getRemote()
  }
  return rewriterService;
}

export const useRewriterContext = () => React.useContext(RewriterContext)

export default function Context(props: React.PropsWithChildren) {
  const [initialText, setInitialText] = React.useState('');
  React.useEffect(() => {
    getRewriterService()
      .getInitialText()
      .then(({ initialText }) => setInitialText(initialText))
  }, [])

  const context = React.useMemo<Context>(() => ({
    initialText,
    setInitialText,

    undo: () => { },
    canUndo: false,

    redo: () => { },
    canRedo: false,

    erase: () => { },
    canErase: false,

    generate: () => { },

    close: () => getRewriterService().close(),

    openSettings: () => getRewriterService().openSettings()
  }), [initialText, close])

  return <RewriterContext.Provider value={context}>
    {props.children}
  </RewriterContext.Provider>
}
