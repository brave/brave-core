import React, { PropTypes, Component } from 'react';
import classnames from 'classnames';
import { SHOW_ALL, SHOW_COMPLETED, SHOW_ACTIVE } from '../constants/TodoFilters';
import style from './Footer.css';

const FILTERS = [SHOW_ALL, SHOW_ACTIVE, SHOW_COMPLETED];
const FILTER_TITLES = {
  [SHOW_ALL]: 'All',
  [SHOW_ACTIVE]: 'Active',
  [SHOW_COMPLETED]: 'Completed'
};

export default class Footer extends Component {

  static propTypes = {
    completedCount: PropTypes.number.isRequired,
    activeCount: PropTypes.number.isRequired,
    filter: PropTypes.string.isRequired,
    onClearCompleted: PropTypes.func.isRequired,
    onShow: PropTypes.func.isRequired
  };

  constructor(props, context) {
    super(props, context);
    if (props.onShow) {
      this.filterHandlers = FILTERS.map(filter => () => props.onShow(filter));
    }
  }

  componentWillReceiveProps(nextProps) {
    if (nextProps.onShow) {
      this.filterHandlers = FILTERS.map(filter => () => nextProps.onShow(filter));
    }
  }

  renderTodoCount() {
    const { activeCount } = this.props;
    const itemWord = activeCount === 1 ? 'item' : 'items';

    return (
      <span className={style.todoCount}>
        <strong>{activeCount || 'No'}</strong> {itemWord} left
      </span>
    );
  }

  renderFilterLink(filter, handler) {
    const title = FILTER_TITLES[filter];
    const { filter: selectedFilter } = this.props;

    return (
      <a
        className={classnames({ selected: filter === selectedFilter })}
        style={{ cursor: 'hand' }}
        onClick={handler}
      >
        {title}
      </a>
    );
  }

  renderClearButton() {
    const { completedCount, onClearCompleted } = this.props;
    if (completedCount > 0) {
      return (
        <button
          className={style.clearCompleted}
          onClick={onClearCompleted}
        >
          Clear completed
        </button>
      );
    }
  }

  render() {
    return (
      <footer className={style.footer}>
        {this.renderTodoCount()}
        <ul className={style.filters}>
          {FILTERS.map((filter, i) =>
            <li key={filter}>
              {this.renderFilterLink(filter, this.filterHandlers[i])}
            </li>
          )}
        </ul>
        {this.renderClearButton()}
      </footer>
    );
  }
}
