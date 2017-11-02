import { expect } from 'chai';
import sinon from 'sinon';
import React from 'react';
import TestUtils from 'react-addons-test-utils';
import Header from '../../../app/components/Header';
import TodoTextInput from '../../../app/components/TodoTextInput';

function setup() {
  const props = {
    addTodo: sinon.spy()
  };

  const renderer = TestUtils.createRenderer();
  renderer.render(<Header {...props} />);
  const output = renderer.getRenderOutput();

  return { props, output, renderer };
}

describe('todoapp Header component', () => {
  it('should render correctly', () => {
    const { output } = setup();

    expect(output.type).to.equal('header');

    const [h1, input] = output.props.children;

    expect(h1.type).to.equal('h1');
    expect(h1.props.children).to.equal('todos');

    expect(input.type).to.equal(TodoTextInput);
    expect(input.props.newTodo).to.equal(true);
    expect(input.props.placeholder).to.equal('What needs to be done?');
  });

  it('should call addTodo if length of text is greater than 0', () => {
    const { output, props } = setup();
    const input = output.props.children[1];
    input.props.onSave('');
    expect(props.addTodo.callCount).to.equal(0);
    input.props.onSave('Use Redux');
    expect(props.addTodo.callCount).to.equal(1);
  });
});
