import React from 'react'
import { render } from 'react-dom'
import { Provider } from 'react-redux'
import { createStore } from 'redux'
import reducers from './reducers'
import App from './components/app'

let store = createStore(reducers)

render(
  <Provider store={store}>
    <App />
  </Provider>,
  document.getElementById('root')
)

console.log('brave_new_tab.js loaded')
