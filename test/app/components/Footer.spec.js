import { expect } from 'chai';
import sinon from 'sinon';
import React from 'react';
import TestUtils from 'react-addons-test-utils';
import Footer from '../../../app/components/Footer';
import style from '../../../app/components/Footer.css';
import { SHOW_ALL, SHOW_ACTIVE } from '../../../app/constants/TodoFilters';

function setup(propOverrides) {
  const props = {
    completedCount: 0,
    activeCount: 0,
    filter: SHOW_ALL,
    onClearCompleted: sinon.spy(),
    onShow: sinon.spy(),
    ...propOverrides
  };

  const renderer = TestUtils.createRenderer();
  renderer.render(<Footer {...props} />);
  const output = renderer.getRenderOutput();

  return { props, output };
}

function getTextContent(elem) {
  const children = Array.isArray(elem.props.children) ?
    elem.props.children : [elem.props.children];

  return children.reduce((out, child) =>
    // Children are either elements or text strings
    out + (child.props ? getTextContent(child) : child)
  , '');
}

describe('todoapp Footer component', () => {
  it('should render correctly', () => {
    const { output } = setup();
    expect(output.type).to.equal('footer');
    expect(output.props.className).to.equal(style.footer);
  });

  it('should display active count when 0', () => {
    const { output } = setup({ activeCount: 0 });
    const [count] = output.props.children;
    expect(getTextContent(count)).to.equal('No items left');
  });

  it('should display active count when above 0', () => {
    const { output } = setup({ activeCount: 1 });
    const [count] = output.props.children;
    expect(getTextContent(count)).to.equal('1 item left');
  });

  it('should render filters', () => {
    const { output } = setup();
    const [, filters] = output.props.children;
    expect(filters.type).to.equal('ul');
    expect(filters.props.className).to.equal(style.filters);
    expect(filters.props.children.length).to.equal(3);
    filters.props.children.forEach((filter, index) => {
      expect(filter.type).to.equal('li');
      const link = filter.props.children;
      expect(link.props.className).to.equal(index === 0 ? 'selected' : '');
      expect(link.props.children).to.equal(['All', 'Active', 'Completed'][index]);
    });
  });

  it('should call onShow when a filter is clicked', () => {
    const { output, props } = setup();
    const [, filters] = output.props.children;
    const filterLink = filters.props.children[1].props.children;
    filterLink.props.onClick({});
    expect(props.onShow.calledWith(SHOW_ACTIVE)).to.equal(true);
  });

  it('shouldnt show clear button when no completed todos', () => {
    const { output } = setup({ completedCount: 0 });
    const [,, clear] = output.props.children;
    expect(clear).to.equal(undefined);
  });

  it('should render clear button when completed todos', () => {
    const { output } = setup({ completedCount: 1 });
    const [,, clear] = output.props.children;
    expect(clear.type).to.equal('button');
    expect(clear.props.children).to.equal('Clear completed');
  });

  it('should call onClearCompleted on clear button click', () => {
    const { output, props } = setup({ completedCount: 1 });
    const [,, clear] = output.props.children;
    clear.props.onClick({});
    expect(props.onClearCompleted.called).to.equal(true);
  });
});
