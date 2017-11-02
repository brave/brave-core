const tasks = require('./tasks');

tasks.replaceWebpack();
console.log('[Copy assets]');
console.log('-'.repeat(80));
tasks.copyAssets('build');

console.log('[Webpack Build]');
console.log('-'.repeat(80));
exec('webpack --config webpack/prod.config.js --progress --profile --colors');
