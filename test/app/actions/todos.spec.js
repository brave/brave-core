import { expect } from 'chai';
import * as types from '../../../app/constants/ActionTypes';
import * as actions from '../../../app/actions/todos';

describe('todoapp todo actions', () => {
  it('addTodo should create ADD_TODO action', () => {
    expect(actions.addTodo('Use Redux')).to.eql({
      type: types.ADD_TODO,
      text: 'Use Redux'
    });
  });

  it('deleteTodo should create DELETE_TODO action', () => {
    expect(actions.deleteTodo(1)).to.eql({
      type: types.DELETE_TODO,
      id: 1
    });
  });

  it('editTodo should create EDIT_TODO action', () => {
    expect(actions.editTodo(1, 'Use Redux everywhere')).to.eql({
      type: types.EDIT_TODO,
      id: 1,
      text: 'Use Redux everywhere'
    });
  });

  it('completeTodo should create COMPLETE_TODO action', () => {
    expect(actions.completeTodo(1)).to.eql({
      type: types.COMPLETE_TODO,
      id: 1
    });
  });

  it('completeAll should create COMPLETE_ALL action', () => {
    expect(actions.completeAll()).to.eql({
      type: types.COMPLETE_ALL
    });
  });

  it('clearCompleted should create CLEAR_COMPLETED action', () => {
    expect(actions.clearCompleted('Use Redux')).to.eql({
      type: types.CLEAR_COMPLETED
    });
  });
});
