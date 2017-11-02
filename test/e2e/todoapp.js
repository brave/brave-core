import path from 'path';
import webdriver from 'selenium-webdriver';
import { expect } from 'chai';
import { delay, startChromeDriver, buildWebDriver } from '../func';
import footerStyle from '../../app/components/Footer.css';
import mainSectionStyle from '../../app/components/MainSection.css';
import todoItemStyle from '../../app/components/TodoItem.css';
import todoTextInputStyle from '../../app/components/TodoTextInput.css';
import manifest from '../../chrome/manifest.prod.json';

const extensionName = manifest.name;

const findList = driver =>
  driver.findElements(webdriver.By.css(`.${mainSectionStyle.todoList} > li`));

const addTodo = async (driver, key) => {
  // add todo
  driver.findElement(webdriver.By.className(todoTextInputStyle.new))
    .sendKeys(key + webdriver.Key.RETURN);
  await delay(1000);
  const todos = await findList(driver);
  return { todo: todos[0], count: todos.length };
};

const editTodo = async (driver, index, key) => {
  let todos = await findList(driver);
  const label = todos[index].findElement(webdriver.By.tagName('label'));
  // dbl click to enable textarea
  await driver.actions().doubleClick(label).perform();
  await delay(500);
  // typing & enter
  driver.actions().sendKeys(key + webdriver.Key.RETURN).perform();
  await delay(1000);

  todos = await findList(driver);
  return { todo: todos[index], count: todos.length };
};

const completeTodo = async (driver, index) => {
  let todos = await findList(driver);
  todos[index].findElement(webdriver.By.className(todoItemStyle.toggle)).click();
  await delay(1000);
  todos = await findList(driver);
  return { todo: todos[index], count: todos.length };
};

const deleteTodo = async (driver, index) => {
  let todos = await findList(driver);
  driver.executeScript(
    `document.querySelectorAll('.${mainSectionStyle.todoList} > li')[${index}]
      .getElementsByClassName('${todoItemStyle.destroy}')[0].style.display = 'block'`
  );
  todos[index].findElement(webdriver.By.className(todoItemStyle.destroy)).click();
  await delay(1000);
  todos = await findList(driver);
  return { count: todos.length };
};

describe('window (popup) page', function test() {
  let driver;
  this.timeout(15000);

  before(async () => {
    await startChromeDriver();
    const extPath = path.resolve('build');
    driver = buildWebDriver(extPath);
    await driver.get('chrome://extensions-frame');
    const elems = await driver.findElements(webdriver.By.xpath(
      '//div[contains(@class, "extension-list-item-wrapper") and ' +
      `.//h2[contains(text(), "${extensionName}")]]`
    ));
    const extensionId = await elems[0].getAttribute('id');
    await driver.get(`chrome-extension://${extensionId}/window.html`);
  });

  after(async () => driver.quit());

  it('should open Redux TodoMVC Example', async () => {
    const title = await driver.getTitle();
    expect(title).to.equal('Redux TodoMVC Example (Window)');
  });

  it('should can add todo', async () => {
    const { todo, count } = await addTodo(driver, 'Add tests');
    expect(count).to.equal(2);
    const text = await todo.findElement(webdriver.By.tagName('label')).getText();
    expect(text).to.equal('Add tests');
  });

  it('should can edit todo', async () => {
    const { todo, count } = await editTodo(driver, 0, 'Ya ');
    expect(count).to.equal(2);
    const text = await todo.findElement(webdriver.By.tagName('label')).getText();
    expect(text).to.equal('Add testsYa');
  });

  it('should can complete todo', async () => {
    const { todo, count } = await completeTodo(driver, 0);
    expect(count).to.equal(2);
    const className = await todo.getAttribute('class');
    const { completed, normal } = todoItemStyle;
    expect(className).to.equal(`${completed} ${normal}`);
  });

  it('should can complete all todos', async () => {
    driver.findElement(webdriver.By.className(mainSectionStyle.toggleAll)).click();
    const todos = await findList(driver);
    const classNames = await Promise.all(todos.map(todo => todo.getAttribute('class')));
    const { completed, normal } = todoItemStyle;
    expect(classNames.every(name => name === `${completed} ${normal}`)).to.equal(true);
  });

  it('should can delete todo', async () => {
    const { count } = await deleteTodo(driver, 0);
    expect(count).to.equal(1);
  });

  it('should can clear completed todos if completed todos count > 0', async () => {
    // current todo count: 1
    await addTodo(driver, 'Add 1');
    const { count } = await addTodo(driver, 'Add 2');
    expect(count).to.equal(3);

    await completeTodo(driver, 0);
    driver.findElement(webdriver.By.className(footerStyle.clearCompleted)).click();

    const todos = await findList(driver);
    const classNames = await Promise.all(todos.map(todo => todo.getAttribute('class')));
    expect(classNames.every(name => name !== 'completed')).to.equal(true);
  });

  it('should cannot clear completed todos if completed todos count = 0', async () => {
    const todos = await driver.findElements(webdriver.By.className(footerStyle.clearCompleted));
    expect(todos.length).to.equal(0);
  });

  it('should can filter active todos', async () => {
    // current todo count: 2
    await addTodo(driver, 'Add 1');
    const { count } = await addTodo(driver, 'Add 2');
    expect(count).to.equal(3);

    await completeTodo(driver, 0);
    let todos = await driver.findElements(webdriver.By.css(`.${footerStyle.filters} > li`));
    todos[1].click();
    await delay(1000);
    todos = await findList(driver);
    expect(todos.length).to.equal(2);
  });

  it('should can filter completed todos', async () => {
    // current todo count: 2
    let todos = await driver.findElements(webdriver.By.css(`.${footerStyle.filters} > li`));
    todos[2].click();
    await delay(1000);
    todos = await findList(driver);
    expect(todos.length).to.equal(1);
  });
});
