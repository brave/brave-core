import React, { Component } from 'react'
import TodoTextInput from './TodoTextInput'

export default class Header extends Component {
  constructor (props, context) {
    super(props, context)
    this.handleSave = this.handleSave.bind(this)
  }
  handleSave (text) {
    if (text.length !== 0) {
      this.props.addTodo(text)
      console.log('this.props added todo')
    }
  }

  render () {
    return (
      <header>
        <h1>todos</h1>
        <TodoTextInput
          newTodo
          onSave={this.handleSave}
          placeholder='What needs to be done?'
        />
      </header>
    )
  }
}
