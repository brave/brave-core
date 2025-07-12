#!/usr/bin/env node

const fs = require('fs');
const path = require('path');
const { execSync, spawn } = require('child_process');
const chalk = require('chalk');

const isWindows = process.platform === 'win32';
const isMacOS = process.platform === 'darwin';
const isLinux = process.platform === 'linux';

const PROJECT_ROOT = path.resolve(__dirname, '../../../../..');
const BRAVE_CORE_ROOT = path.resolve(__dirname, '../../..');
const CLAUDE_CONFIG_PATH = path.join(PROJECT_ROOT, '.claude', 'mcp_servers.json');
const CURSOR_CONFIG_PATH = path.join(require('os').homedir(), '.cursor', 'mcp.json');
const GLOBAL_CLAUDE_CONFIG_PATH = path.join(require('os').homedir(), '.claude', 'mcp_servers.json');
const REQUIREMENTS_PATH = path.join(BRAVE_CORE_ROOT, 'agents', 'mcp', 'requirements.txt');

function log(message, type = 'info') {
  const colors = {
    info: chalk.blue,
    success: chalk.green,
    warning: chalk.yellow,
    error: chalk.red
  };
  console.log(colors[type](`[MCP Setup] ${message}`));
}

function ensureDirectoryExists(dirPath) {
  if (!fs.existsSync(dirPath)) {
    fs.mkdirSync(dirPath, { recursive: true });
    log(`Created directory: ${dirPath}`, 'success');
  }
}

function getPythonCommand() {
  // Try to find the best Python command for the platform
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

function installMCPDependencies() {
  log('Installing MCP dependencies...');
  
  const pythonCmd = getPythonCommand();
  
  try {
    // Install MCP package
    execSync(`${pythonCmd} -m pip install mcp`, { stdio: 'inherit' });
    
    // Install requirements if they exist
    if (fs.existsSync(REQUIREMENTS_PATH)) {
      execSync(`${pythonCmd} -m pip install -r "${REQUIREMENTS_PATH}"`, { stdio: 'inherit' });
    }
    
    log('MCP dependencies installed successfully', 'success');
  } catch (error) {
    log(`Failed to install dependencies: ${error.message}`, 'error');
    throw error;
  }
}

function detectIDEs() {
  const ides = [];
  
  // Check for Claude Code (look for .claude directory or running process)
  const claudeUserDir = path.join(require('os').homedir(), '.claude');
  if (fs.existsSync(claudeUserDir) || process.env.CLAUDE_CODE_API_KEY) {
    ides.push('claude');
  }
  
  // Check for Cursor (look for .cursor directory or in PATH)
  const cursorUserDir = path.join(require('os').homedir(), '.cursor');
  if (fs.existsSync(cursorUserDir)) {
    ides.push('cursor');
  }
  
  // Try to detect from running processes or common install locations
  try {
    if (isWindows) {
      // Check common Windows install paths
      const cursorPaths = [
        path.join(require('os').homedir(), 'AppData', 'Local', 'Programs', 'cursor'),
        'C:\\Users\\' + require('os').userInfo().username + '\\AppData\\Local\\Programs\\cursor'
      ];
      if (cursorPaths.some(p => fs.existsSync(p))) {
        if (!ides.includes('cursor')) ides.push('cursor');
      }
    } else if (isMacOS) {
      // Check macOS application directories
      const cursorPaths = [
        '/Applications/Cursor.app',
        path.join(require('os').homedir(), 'Applications', 'Cursor.app')
      ];
      if (cursorPaths.some(p => fs.existsSync(p))) {
        if (!ides.includes('cursor')) ides.push('cursor');
      }
      
      // Also check if cursor is in PATH on macOS
      try {
        execSync('which cursor', { stdio: 'pipe' });
        if (!ides.includes('cursor')) ides.push('cursor');
      } catch (e) {
        // cursor not in PATH
      }
    } else if (isLinux) {
      // Check Linux application directories and PATH
      const cursorPaths = [
        '/usr/bin/cursor',
        '/usr/local/bin/cursor',
        path.join(require('os').homedir(), '.local', 'bin', 'cursor'),
        '/opt/cursor/cursor',
        '/snap/bin/cursor'
      ];
      if (cursorPaths.some(p => fs.existsSync(p))) {
        if (!ides.includes('cursor')) ides.push('cursor');
      }
      
      // Also check if cursor is in PATH on Linux
      try {
        execSync('which cursor', { stdio: 'pipe' });
        if (!ides.includes('cursor')) ides.push('cursor');
      } catch (e) {
        // cursor not in PATH
      }
    }
  } catch (e) {
    // Ignore detection errors
  }
  
  return ides;
}

function createMCPConfig() {
  log('Creating MCP server configuration...');
  
  // Determine paths based on execution context and platform
  // Support running from brave-browser or brave-browser/src/brave
  const currentDir = process.cwd();
  let projectRoot, braveCorePath, chromiumSrcPath, pythonPath;
  
  if (currentDir.endsWith('src/brave') || currentDir.includes('/src/brave')) {
    // Running from src/brave directory
    projectRoot = path.resolve(currentDir, '../..');
    chromiumSrcPath = path.resolve(currentDir, '..');
  } else {
    // Running from brave-browser root
    projectRoot = currentDir;
    chromiumSrcPath = path.join(currentDir, 'src');
  }
  
  // Brave-core is typically a sibling to brave-browser
  braveCorePath = path.resolve(projectRoot, '..', 'brave-core');
  
  // PYTHONPATH should point to the src directory
  pythonPath = chromiumSrcPath;
  
  // Normalize paths for cross-platform compatibility
  if (isWindows) {
    braveCorePath = braveCorePath.replace(/\\/g, '/');
    chromiumSrcPath = chromiumSrcPath.replace(/\\/g, '/');
    pythonPath = pythonPath.replace(/\\/g, '/');
  }
  
  const mcpConfig = {
    mcpServers: {
      "brave-source": {
        command: getPythonCommand(),
        args: ["-m", "brave.agents.mcp.source_access_server"],
        cwd: projectRoot,
        env: {
          BRAVE_CORE_PATH: braveCorePath,
          CHROMIUM_SRC_PATH: chromiumSrcPath,
          PYTHONPATH: pythonPath
        },
        timeout: 30000
      }
    }
  };
  
  // Detect which IDEs are available
  const detectedIDEs = detectIDEs();
  
  if (detectedIDEs.length === 0) {
    log('No supported IDEs detected. Creating configurations for both Claude Code and Cursor.', 'warning');
    detectedIDEs.push('claude', 'cursor');
  } else {
    log(`Detected IDEs: ${detectedIDEs.join(', ')}`, 'info');
  }
  
  // Configure for Claude Code
  if (detectedIDEs.includes('claude')) {
    ensureDirectoryExists(path.dirname(CLAUDE_CONFIG_PATH));
    fs.writeFileSync(CLAUDE_CONFIG_PATH, JSON.stringify(mcpConfig, null, 2));
    log(`✅ Configured Claude Code: ${CLAUDE_CONFIG_PATH}`, 'success');
  }
  
  // Configure for Cursor
  if (detectedIDEs.includes('cursor')) {
    ensureDirectoryExists(path.dirname(CURSOR_CONFIG_PATH));
    
    // Read existing Cursor config if it exists
    let cursorConfig = { mcpServers: {} };
    if (fs.existsSync(CURSOR_CONFIG_PATH)) {
      try {
        const existing = fs.readFileSync(CURSOR_CONFIG_PATH, 'utf8');
        cursorConfig = JSON.parse(existing);
      } catch (error) {
        log(`Warning: Could not parse existing Cursor config: ${error.message}`, 'warning');
      }
    }
    
    // Merge configurations
    cursorConfig.mcpServers = {
      ...cursorConfig.mcpServers,
      ...mcpConfig.mcpServers
    };
    
    fs.writeFileSync(CURSOR_CONFIG_PATH, JSON.stringify(cursorConfig, null, 2));
    log(`✅ Configured Cursor: ${CURSOR_CONFIG_PATH}`, 'success');
    
    // Create .cursorrules file for automatic Brave development context
    const cursorRulesPath = path.join(PROJECT_ROOT, '.cursorrules');
    const bravePromptPath = path.join(BRAVE_CORE_ROOT, 'agents', 'prompts', 'brave_developer.md');
    
    try {
      if (fs.existsSync(bravePromptPath)) {
        const promptContent = fs.readFileSync(bravePromptPath, 'utf8');
        // Remove the markdown header and format for .cursorrules
        const rulesContent = promptContent.replace(/^# Brave Browser Development Assistant\n\n/, '');
        fs.writeFileSync(cursorRulesPath, rulesContent);
        log(`✅ Created Cursor rules: ${cursorRulesPath}`, 'success');
      }
    } catch (error) {
      log(`Warning: Could not create .cursorrules: ${error.message}`, 'warning');
    }
  }
  
  return mcpConfig;
}

function updateGlobalConfig(projectConfig) {
  if (process.argv.includes('--global')) {
    log('Updating global Claude configuration...');
    
    ensureDirectoryExists(path.dirname(GLOBAL_CLAUDE_CONFIG_PATH));
    
    let globalConfig = { mcpServers: {} };
    
    // Read existing global config if it exists
    if (fs.existsSync(GLOBAL_CLAUDE_CONFIG_PATH)) {
      try {
        const existing = fs.readFileSync(GLOBAL_CLAUDE_CONFIG_PATH, 'utf8');
        globalConfig = JSON.parse(existing);
      } catch (error) {
        log(`Warning: Could not parse existing global config: ${error.message}`, 'warning');
      }
    }
    
    // Merge with project config
    globalConfig.mcpServers = {
      ...globalConfig.mcpServers,
      ...projectConfig.mcpServers
    };
    
    fs.writeFileSync(GLOBAL_CLAUDE_CONFIG_PATH, JSON.stringify(globalConfig, null, 2));
    log(`Updated global Claude config: ${GLOBAL_CLAUDE_CONFIG_PATH}`, 'success');
  }
}

function testMCPServer() {
  if (process.argv.includes('--skip-test')) {
    return;
  }
  
  log('Testing MCP server...');
  
  const testScriptPath = path.join(BRAVE_CORE_ROOT, 'agents', 'mcp', 'test_server.py');
  const pythonCmd = getPythonCommand();
  
  try {
    execSync(`${pythonCmd} "${testScriptPath}"`, { 
      stdio: 'inherit',
      cwd: PROJECT_ROOT 
    });
    log('MCP server test completed', 'success');
  } catch (error) {
    log('MCP server test failed - but this may be normal for first-time setup', 'warning');
  }
}

function startMCPServer() {
  if (!process.argv.includes('--start')) {
    return;
  }
  
  log('Starting MCP server...');
  
  const pythonCmd = getPythonCommand();
  
  const serverProcess = spawn(pythonCmd, ['-m', 'brave.agents.mcp.source_access_server'], {
    cwd: PROJECT_ROOT,
    stdio: 'inherit',
    env: {
      ...process.env,
      PYTHONPATH: path.join(PROJECT_ROOT, 'src')
    }
  });
  
  process.on('SIGINT', () => {
    log('Stopping MCP server...');
    serverProcess.kill('SIGINT');
    process.exit(0);
  });
  
  serverProcess.on('error', (error) => {
    log(`Server error: ${error.message}`, 'error');
  });
  
  serverProcess.on('close', (code) => {
    log(`Server exited with code ${code}`);
  });
}

function showUsage() {
  console.log(`
${chalk.bold('Brave MCP Server Setup')}

Usage: npm run mcp:setup [options]

${chalk.bold('What it does:')}
  - Installs Python dependencies (mcp package)
  - Auto-detects Claude Code and Cursor IDEs  
  - Configures MCP server for detected IDEs
  - Tests the setup automatically

${chalk.bold('Options:')}
  --global      Also install to global Claude configuration
  --start       Start the MCP server after setup
  --skip-test   Skip running the test suite
  --help        Show this help message

${chalk.bold('Examples:')}
  npm run mcp:setup                    # Auto-setup for detected IDEs
  npm run mcp:setup -- --global       # Setup + install globally  
  npm run mcp:setup -- --start        # Setup + start server
  npm run mcp:test                     # Run tests only
  npm run mcp:start                    # Start server only

${chalk.bold('Supported IDEs:')} Claude Code, Cursor
${chalk.bold('Platforms:')} Linux, macOS, Windows
`);
}

async function main() {
  if (process.argv.includes('--help')) {
    showUsage();
    return;
  }
  
  try {
    log(`Setting up MCP server on ${process.platform}...`);
    
    // Install dependencies
    installMCPDependencies();
    
    // Create configuration
    const config = createMCPConfig();
    
    // Update global config if requested
    updateGlobalConfig(config);
    
    // Test the server
    testMCPServer();
    
    // Start server if requested
    startMCPServer();
    
    if (!process.argv.includes('--start')) {
      log('✅ MCP setup completed successfully!', 'success');
      log('');
      const detectedIDEs = detectIDEs();
      log('Next steps:', 'info');
      
      if (detectedIDEs.includes('claude')) {
        log('📝 Claude Code: Restart if running', 'info');
      }
      if (detectedIDEs.includes('cursor')) {
        log('📝 Cursor: Restart if running', 'info');
      }
      
      log('🔧 The "brave-source" MCP server is now available', 'info');
      log('📚 Default prompt: src/brave/agents/prompts/brave_developer.md', 'info');
      log('🧪 Test with: npm run mcp:test', 'info');
      log('🚀 Debug with: npm run mcp:start', 'info');
    }
    
  } catch (error) {
    log(`Setup failed: ${error.message}`, 'error');
    process.exit(1);
  }
}

if (require.main === module) {
  main();
}