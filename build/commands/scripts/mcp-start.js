#!/usr/bin/env node

const { spawn } = require('child_process');
const path = require('path');
const chalk = require('chalk');

const PROJECT_ROOT = path.resolve(__dirname, '../../../../..');

function log(message, type = 'info') {
  const colors = {
    info: chalk.blue,
    success: chalk.green,
    warning: chalk.yellow,
    error: chalk.red
  };
  console.log(colors[type](`[MCP Server] ${message}`));
}

function getPythonCommand() {
  const pythonCommands = ['python3', 'python'];
  const { execSync } = require('child_process');
  
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
    
    log('Starting Brave source MCP server...');
    log('Press Ctrl+C to stop the server');
    
    const serverProcess = spawn(pythonCmd, ['-m', 'brave.agents.mcp.source_access_server'], {
      cwd: PROJECT_ROOT,
      stdio: 'inherit',
      env: {
        ...process.env,
        PYTHONPATH: PROJECT_ROOT + "/src"
      }
    });
    
    process.on('SIGINT', () => {
      log('Stopping MCP server...');
      serverProcess.kill('SIGINT');
      process.exit(0);
    });
    
    process.on('SIGTERM', () => {
      log('Stopping MCP server...');
      serverProcess.kill('SIGTERM');
      process.exit(0);
    });
    
    serverProcess.on('error', (error) => {
      log(`Server error: ${error.message}`, 'error');
      process.exit(1);
    });
    
    serverProcess.on('close', (code) => {
      if (code !== 0) {
        log(`Server exited with code ${code}`, code === 0 ? 'info' : 'error');
      }
      process.exit(code);
    });
    
  } catch (error) {
    log(`Failed to start server: ${error.message}`, 'error');
    process.exit(1);
  }
}

if (require.main === module) {
  main();
}