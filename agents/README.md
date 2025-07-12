# Brave AI Development Tools

AI-powered development tools for Brave browser development. Provides intelligent source code navigation across the massive Brave + Chromium codebase.

## Quick Start

```bash
npm run mcp:setup  # Install and configure MCP server
npm run mcp:test   # Verify everything works
```

## What This Provides

- **MCP Server**: Navigate 2M+ files across Brave-core and Chromium repositories
- **IDE Integration**: Auto-configures Claude Code and Cursor 
- **Intelligent Search**: Find files, code patterns, and Brave-specific components
- **Development Context**: Specialized prompt with Brave architecture knowledge

## Available Tools

- `search_files(pattern, directory)` - Find files by name pattern
- `search_code(query, file_pattern, directory)` - Search code content  
- `get_file_info(file_path)` - Get detailed file information
- `find_brave_components(component_name)` - Locate Brave-specific components

## IDE Setup

**Claude Code**: Automatically configured - just restart if running

**Cursor**: 
- **Option 1**: Uses `.cursorrules` (automatic setup)
- **Option 2**: Reference `@agents/prompts/brave_developer.md` manually

## Repository Structure

```
brave-browser/          # Main build repository  
├── src/                # Chromium source code (full fork)
│   ├── brave/          # Brave-core repository
│   │   ├── components/ # Brave-specific components
│   │   └── agents/     # AI development tools (this directory)
│   ├── chrome/         # Chromium browser code (patched)
│   └── ...            # Other Chromium directories
└── package.json        # Build scripts
```

## Manual Configuration

If automatic setup fails:

1. Install: `pip install mcp`
2. Configure IDE with MCP server pointing to `brave.agents.mcp.source_access_server`
3. Set environment variables for Brave-core and Chromium paths
4. Restart IDE

## Files

- `mcp/` - Model Context Protocol server implementation
- `prompts/brave_developer.md` - Specialized development prompt with MCP tool guidance
- `build/commands/scripts/` - Setup and management scripts

## Troubleshooting

- **Server not appearing**: Restart IDE completely
- **MCP package not found**: Run `pip install mcp`  
- **Debug mode**: Run `npm run mcp:start` to test manually
- **Path issues**: Ensure brave-core is alongside brave-browser

Focus on practical development assistance with comprehensive codebase access.