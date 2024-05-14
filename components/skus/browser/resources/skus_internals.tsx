// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as SkusInternalsMojo from 'gen/brave/components/skus/common/skus_internals.mojom.m.js'
import * as React from 'react'
import styled from 'styled-components'
import { render } from 'react-dom'
import { JsonView } from 'react-json-view-lite';
import 'react-json-view-lite/dist/index.css';

const Container = styled.div`
  margin: 4px;
  display: flex;
  flex-direction: column;
  gap: 10px;
`

const ButtonContainer = styled.div`
  display: flex;
  flex-direction: row;
  gap: 4px;
`

const StateContainer = styled.div`
  padding: 4px;
`

const API = SkusInternalsMojo.SkusInternals.getRemote()

interface CreateOrderFromReceiptInputFields {
  domain: string
  receipt: string
  order_id: string
}

const defaultState: CreateOrderFromReceiptInputFields = {
  domain: 'vpn.brave.com',
  receipt: '',
  order_id: ''
}

function App() {
  const [skusState, setSkusState] = React.useState({})
  const [vpnState, setVpnState] = React.useState({})
  const [leoState, setLeoState] = React.useState({})
  const [createOrderFormData, setCreateOrderFormData] = React.useState<CreateOrderFromReceiptInputFields>(defaultState)

  const resetSkusState = () => {
    API.resetSkusState()
    setSkusState({})
    setVpnState({})
    setLeoState({})
  }

  const getSkusState = () => {
    API.getSkusState().then((r: any) => {
      setSkusState(JSON.parse(r.response))
    })
  }

  const getVpnState = () => {
    API.getVpnState().then((r: any) => {
      setVpnState(JSON.parse(r.response))
    })
  }

  const getLeoState = () => {
    API.getLeoState().then((r: any) => {
      setLeoState(JSON.parse(r.response))
    })
  }

  const submitCreateOrderFromReceipt = () => {
    API.createOrderFromReceipt(
      createOrderFormData.domain, createOrderFormData.receipt).then((r: any) =>
    {
      console.log(r)
      console.log(r.response)
      // TODO(bsclifton): show/set order ID here
    })
  }

  type FormElement = HTMLInputElement | HTMLTextAreaElement | HTMLSelectElement
  type BaseType = string | number | React.FormEvent<FormElement>
  function getOnChangeField<T extends BaseType = BaseType> (key: keyof CreateOrderFromReceiptInputFields) {
    return function (e: T) {
      const value = (typeof e === 'string' || typeof e === 'number') ? e : e.currentTarget.value
      if (createOrderFormData[key] === value) {
        return
      }
      setCreateOrderFormData(data => ({
        ...data,
        [key]: value
      }))
    }
  }

  return (
    <Container>
      <h2>SKUs internals</h2>
      <ButtonContainer>
        <button onClick={() => {
            if (window.confirm('Resetting SKUs state will count the next sign in for account.brave.com as a new device. If you do this more than once you may run out of devices/credentials. Are you sure you want to reset SKUs state?')) {
              resetSkusState()
            }
          }}>
          Reset SKUs state
        </button>
        <button onClick={getVpnState}>Fetch VPN state</button>
        <button onClick={getLeoState}>Fetch Leo state</button>
        <button onClick={getSkusState}>Fetch SKUs state</button>
      </ButtonContainer>

      <StateContainer>
        <b>VPN State:</b>
        <JsonView data={vpnState} shouldInitiallyExpand={(level) => true} />
      </StateContainer>

      <StateContainer>
        <b>Leo State:</b>
        <JsonView data={leoState} shouldInitiallyExpand={(level) => true} />
      </StateContainer>

      <StateContainer>
        <b>SKUs State:</b>
        <button onClick={() => API.copySkusStateToClipboard()}>Copy</button>
        <button onClick={() => API.downloadSkusState()}>Download</button>
        <JsonView data={skusState} shouldInitiallyExpand={(level) => true} />
      </StateContainer>

      <StateContainer>
        <div>
          <b>Create Order From Receipt:</b>
        </div>
        <div>
          <div>
            Domain
          </div>
          <div>
          <input
            type="text"
            value={createOrderFormData.domain}
            onChange={getOnChangeField('domain')}
          />
          </div>
          <div>
            Receipt
          </div>
          <textarea
            style={{ marginTop: '10px' }}
            cols={100}
            rows={10}
            spellCheck={false}
            value={createOrderFormData.receipt}
            onChange={getOnChangeField('receipt')}
          />
        </div>
        <div>
          <button onClick={submitCreateOrderFromReceipt}>Submit</button>
        </div>
        <div>
          {createOrderFormData.order_id}
        </div>
      </StateContainer>
    </Container>
  )
}

document.addEventListener('DOMContentLoaded', () => {
  render(<App />, document.getElementById('root'))
})
