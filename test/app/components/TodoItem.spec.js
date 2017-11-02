import { expect } from 'chai';
import sinon from 'sinon';
import React from 'react';
import TestUtils from 'react-addons-test-utils';
import TodoItem from '../../../app/components/TodoItem';
import style from '../../../app/components/TodoItem.css';
import TodoTextInput from '../../../app/components/TodoTextInput';

function setup(editing = false) {
  const props = {
    todo: {
      id: 0,
      text: 'Use Redux',
      completed: false
    },
    editTodo: sinon.spy(),
    deleteTodo: sinon.spy(),
    completeTodo: sinon.spy()
  };

  const renderer = TestUtils.createRenderer();

  renderer.render(<TodoItem {...props} />);

  let output = renderer.getRenderOutput();

  if (editing) {
    const label = output.props.children.props.children[1];
    label.props.onDoubleClick({});
    output = renderer.getRenderOutput();
  }

  return { props, output, renderer };
}

describe('todoapp TodoItem component', () => {
  it('should render correctly', () => {
    const { output } = setup();

    expect(output.type).to.equal('li');
    expect(output.props.className).to.equal(style.normal);

    const div = output.props.children;

    expect(div.type).to.equal('div');
    expect(div.props.className).to.equal(style.view);

    const [input, label, button] = div.props.children;

    expect(input.type).to.equal('input');
    expect(input.props.checked).to.equal(false);

    expect(label.type).to.equal('label');
    expect(label.props.children).to.equal('Use Redux');

    expect(button.type).to.equal('button');
    expect(button.props.className).to.equal(style.destroy);
  });

  it('input onChange should call completeTodo', () => {
    const { output, props } = setup();
    const input = output.props.children.props.children[0];
    input.props.onChange({});
    expect(props.completeTodo.calledWith(0)).to.equal(true);
  });

  it('button onClick should call deleteTodo', () => {
    const { output, props } = setup();
    const button = output.props.children.props.children[2];
    button.props.onClick({});
    expect(props.deleteTodo.calledWith(0)).to.equal(true);
  });

  it('label onDoubleClick should put component in edit state', () => {
    const { output, renderer } = setup();
    const label = output.props.children.props.children[1];
    label.props.onDoubleClick({});
    const updated = renderer.getRenderOutput();
    expect(updated.type).to.equal('li');
    expect(updated.props.className).to.equal(style.editing);
  });

  it('edit state render', () => {
    const { output } = setup(true);

    expect(output.type).to.equal('li');
    expect(output.props.className).to.equal(style.editing);

    const input = output.props.children;
    expect(input.type).to.equal(TodoTextInput);
    expect(input.props.text).to.equal('Use Redux');
    expect(input.props.editing).to.equal(true);
  });

  it('TodoTextInput onSave should call editTodo', () => {
    const { output, props } = setup(true);
    output.props.children.props.onSave('Use Redux');
    expect(props.editTodo.calledWith(0, 'Use Redux')).to.equal(true);
  });

  it('TodoTextInput onSave should call deleteTodo if text is empty', () => {
    const { output, props } = setup(true);
    output.props.children.props.onSave('');
    expect(props.deleteTodo.calledWith(0)).to.equal(true);
  });

  it('TodoTextInput onSave should exit component from edit state', () => {
    const { output, renderer } = setup(true);
    output.props.children.props.onSave('Use Redux');
    const updated = renderer.getRenderOutput();
    expect(updated.type).to.equal('li');
    expect(updated.props.className).to.equal(style.normal);
  });
});
