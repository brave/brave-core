import { expect } from 'chai';
import sinon from 'sinon';
import React from 'react';
import TestUtils from 'react-addons-test-utils';
import TodoTextInput from '../../../app/components/TodoTextInput';
import style from '../../../app/components/TodoTextInput.css';

function setup(propOverrides) {
  const props = {
    onSave: sinon.spy(),
    text: 'Use Redux',
    placeholder: 'What needs to be done?',
    editing: false,
    newTodo: false,
    ...propOverrides
  };

  const renderer = TestUtils.createRenderer();

  renderer.render(<TodoTextInput {...props} />);

  let output = renderer.getRenderOutput();

  output = renderer.getRenderOutput();

  return { props, output, renderer };
}

describe('todoapp TodoTextInput component', () => {
  it('should render correctly', () => {
    const { output } = setup();
    expect(output.props.placeholder).to.equal('What needs to be done?');
    expect(output.props.value).to.equal('Use Redux');
    expect(output.props.className).to.equal('');
  });

  it('should render correctly when editing=true', () => {
    const { output } = setup({ editing: true });
    expect(output.props.className).to.equal(style.edit);
  });

  it('should render correctly when newTodo=true', () => {
    const { output } = setup({ newTodo: true });
    expect(output.props.className).to.equal(style.new);
  });

  it('should update value on change', () => {
    const { output, renderer } = setup();
    output.props.onChange({ target: { value: 'Use Radox' } });
    const updated = renderer.getRenderOutput();
    expect(updated.props.value).to.equal('Use Radox');
  });

  it('should call onSave on return key press', () => {
    const { output, props } = setup();
    output.props.onKeyDown({ which: 13, target: { value: 'Use Redux' } });
    expect(props.onSave.calledWith('Use Redux')).to.equal(true);
  });

  it('should reset state on return key press if newTodo', () => {
    const { output, renderer } = setup({ newTodo: true });
    output.props.onKeyDown({ which: 13, target: { value: 'Use Redux' } });
    const updated = renderer.getRenderOutput();
    expect(updated.props.value).to.equal('');
  });

  it('should call onSave on blur', () => {
    const { output, props } = setup();
    output.props.onBlur({ target: { value: 'Use Redux' } });
    expect(props.onSave.calledWith('Use Redux')).to.equal(true);
  });

  it('shouldnt call onSave on blur if newTodo', () => {
    const { output, props } = setup({ newTodo: true });
    output.props.onBlur({ target: { value: 'Use Redux' } });
    expect(props.onSave.callCount).to.equal(0);
  });
});
