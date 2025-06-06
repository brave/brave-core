// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import './no_ce'

import * as React from "react";
import { AIChatContext, AIChatReactContext, defaultContext as defaultAIChatContext } from "../../state/ai_chat_context";
import { ConversationReactContext, ConversationContext, defaultContext as defaultConversationContext } from "../../state/conversation_context";
import { render } from "@testing-library/react";
import TabsMenu from "./tabs_menu";
import { ContentType } from 'gen/brave/components/ai_chat/core/common/mojom/ai_chat.mojom.m.js'
const MockContext = (props: React.PropsWithChildren<Partial<AIChatContext & ConversationContext>>) => {
  return <AIChatReactContext.Provider value={{
    ...defaultAIChatContext,
    ...props,
  }}>
    <ConversationReactContext.Provider value={{
      ...defaultConversationContext,
      ...props,
    }}>
      {props.children}
    </ConversationReactContext.Provider>
  </AIChatReactContext.Provider>
}

describe('TabsMenu', () => {
  it('should render tabs', () => {
    const { getByText } = render(<MockContext tabs={[{
      contentId: 1,
      title: 'Test 1',
      url: {
        url: 'https://tes1t.com',
      },
      id: 1
    }, {
      contentId: 2,
      title: 'Test 2',
      url: {
        url: 'https://test2.com',
      },
      id: 2
    }]}>
      <TabsMenu />
    </MockContext>)

    expect(getByText('Test 1')).toBeInTheDocument()
    expect(getByText('Test 2')).toBeInTheDocument()
  })

  it('should filter out attached tabs', () => {
    const { queryByText } = render(<MockContext associatedContentInfo={[
      {
        contentId: 1,
        title: 'Test 1',
        url: {
          url: 'https://test1.com',
        },
        contentType: ContentType.PageContent,
        contentUsedPercentage: 0,
        uuid: '1'
      }
    ]} tabs={[{
      contentId: 1,
      title: 'Test 1',
      url: {
        url: 'https://tes1t.com',
      },
      id: 1
    }, {
      contentId: 2,
      title: 'Test 2',
      url: {
        url: 'https://test2.com',
      },
      id: 2
    }]}>
      <TabsMenu />
    </MockContext>)

    expect(queryByText('Test 1')).not.toBeInTheDocument()
    expect(queryByText('Test 2')).toBeInTheDocument()
  })

  it('should be open when query starts with @', () => {
    const { container } = render(<MockContext inputText='@'>
      <TabsMenu />
    </MockContext>)

    const menu = container.querySelector('leo-buttonmenu')
    expect(menu).toBeInTheDocument()
    expect(menu).toHaveProperty('isOpen', true)
  })

  it('should be close when @ is removed', () => {
    render(<MockContext inputText='@'>
      <TabsMenu />
    </MockContext>)


    const { container } = render(<MockContext inputText='hi'>
      <TabsMenu />
    </MockContext>)

    const menu = container.querySelector('leo-buttonmenu')
    expect(menu).toBeInTheDocument()
    expect(menu).toHaveProperty('isOpen', false)
  })

  it('should filter by text after @', () => {
    const { queryByText } = render(<MockContext inputText='@2' tabs={[{
      contentId: 1,
      title: 'Test 1',
      url: {
        url: 'https://tes1t.com',
      },
      id: 1
    }, {
      contentId: 2,
      title: 'Test 2',
      url: {
        url: 'https://test2.com',
      },
      id: 2
    }]}>
      <TabsMenu />
    </MockContext>)

    expect(queryByText('Test 1')).not.toBeInTheDocument()
    expect(queryByText('Test 2')).toBeInTheDocument()
  })

  it('selecting an element should clear text and attempt to associate with current conversation', () => {
    const associateTab = jest.fn()
    const tab1 = {
      contentId: 1,
      title: 'Test 1',
      url: {
        url: 'https://tes1t.com',
      },
      id: 1
    }
    const { queryByText } = render(<MockContext
      conversationUuid='1'
      inputText='@'
      uiHandler={{
        associateTab,
        ...defaultAIChatContext.uiHandler,
      } as any}
      tabs={[tab1, {
        contentId: 2,
        title: 'Test 2',
        url: {
          url: 'https://test2.com',
        },
        id: 2
      }]}>
      <TabsMenu />
    </MockContext>)

    queryByText('Test 1')?.click()

    expect(associateTab).toHaveBeenCalledWith(tab1, '1')
    expect(queryByText('@')).not.toBeInTheDocument()
  })
})
