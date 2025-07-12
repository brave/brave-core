#!/usr/bin/env node

const { execSync } = require('child_process');
const path = require('path');
const chalk = require('chalk');

const PROJECT_ROOT = path.resolve(__dirname, '../../../../..');
const BRAVE_CORE_ROOT = path.resolve(__dirname, '../../..');
const TEST_SCRIPT_PATH = path.join(BRAVE_CORE_ROOT, 'agents', 'mcp', 'test_server.py');

function log(message, type = 'info') {
  const colors = {
    info: chalk.blue,
    success: chalk.green,
    warning: chalk.yellow,
    error: chalk.red
  };
  console.log(colors[type](`[MCP Test] ${message}`));
}

function getPythonCommand() {
  const pythonCommands = ['python3', 'python'];
  
  for (const cmd of pythonCommands) {
    try {
      execSync(`${cmd} --version`, { stdio: 'pipe' });
      return cmd;
    } catch (e) {
      continue;
    }
  }
  throw new Error('Python not found. Please install Python 3.7+ and ensure it\'s in your PATH.');
}

function main() {
  try {
    const pythonCmd = getPythonCommand();
    
    log('Running MCP server tests...');
    
    execSync(`${pythonCmd} "${TEST_SCRIPT_PATH}"`, { 
      stdio: 'inherit',
      cwd: PROJECT_ROOT 
    });
    
    log('Test completed successfully', 'success');
    
  } catch (error) {
    log(`Test failed: ${error.message}`, 'error');
    process.exit(1);
  }
}

if (require.main === module) {
  main();
}