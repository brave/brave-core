import path from 'path';
import webdriver from 'selenium-webdriver';
import { expect } from 'chai';
import { delay, startChromeDriver, buildWebDriver } from '../func';

describe('inject page (in github.com)', function test() {
  let driver;
  this.timeout(15000);

  before(async () => {
    await startChromeDriver();
    const extPath = path.resolve('build');
    driver = buildWebDriver(extPath);
    await driver.get('https://github.com');
  });

  after(async () => driver.quit());

  it('should open Github', async () => {
    const title = await driver.getTitle();
    expect(title).to.equal('The world\'s leading software development platform · GitHub');
  });

  it('should render inject app', async () => {
    await driver.wait(
      () => driver.findElements(webdriver.By.className('inject-react-example'))
        .then(elems => elems.length > 0),
      10000,
      'Inject app not found'
    );
  });

  it('should find `Open TodoApp` button', async () => {
    await driver.wait(
      () => driver.findElements(webdriver.By.css('.inject-react-example button'))
        .then(elems => elems.length > 0),
      10000,
      'Inject app `Open TodoApp` button not found'
    );
  });

  it('should find iframe', async () => {
    driver.findElement(webdriver.By.css('.inject-react-example button')).click();
    await delay(1000);
    await driver.wait(
      () => driver.findElements(webdriver.By.css('.inject-react-example iframe'))
        .then(elems => elems.length > 0),
      10000,
      'Inject app iframe not found'
    );
  });
});
