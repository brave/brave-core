#!/usr/bin/env python3
"""
Generic MCP server for Brave browser source code access.
Provides tools to search and navigate the Brave + Chromium codebase.
"""

import json
import os
import sys
import subprocess
import asyncio
import re
import shlex
from typing import Any, Dict, List, Optional
from pathlib import Path

try:
    from mcp import ClientSession, StdioServerParameters
    from mcp.server import NotificationOptions, Server
    from mcp.server.models import InitializationOptions
    from mcp.server.stdio import stdio_server
    from mcp.types import (
        Resource,
        Tool,
        TextContent,
        ImageContent,
        EmbeddedResource,
    )
except ImportError:
    print("Error: MCP package not found. Install with: pip install mcp", file=sys.stderr)
    sys.exit(1)

# Server configuration
app = Server("brave-source-access")

# Path configurations
BRAVE_CORE_PATH = os.environ.get("BRAVE_CORE_PATH", "../brave-core")
CHROMIUM_SRC_PATH = os.environ.get("CHROMIUM_SRC_PATH", "./src")
CURRENT_DIR = Path(__file__).parent.parent.parent

# Security configuration
MAX_FILE_SIZE = 10 * 1024 * 1024  # 10MB limit for file reads
MAX_SEARCH_RESULTS = 100  # Limit search results
ALLOWED_FILE_EXTENSIONS = {'.py', '.js', '.ts', '.cc', '.h', '.cpp', '.c', '.hpp', '.java', '.gn', '.gni', '.md', '.json', '.xml', '.html', '.css', '.proto', '.mojom'}

def validate_path(path: str) -> bool:
    """Validate that a path is safe and within allowed directories."""
    try:
        # Resolve path and check it's within allowed bounds
        resolved_path = Path(path).resolve()
        current_dir = CURRENT_DIR.resolve()
        
        # Must be within the current directory tree
        try:
            resolved_path.relative_to(current_dir)
        except ValueError:
            return False
            
        # Block dangerous path components
        path_str = str(resolved_path)
        dangerous_patterns = ['..', '~', '$', '`', ';', '|', '&', '<', '>', '*', '?']
        for pattern in dangerous_patterns:
            if pattern in path:
                return False
                
        return True
    except (OSError, ValueError):
        return False

def sanitize_search_input(input_str: str) -> str:
    """Sanitize search input to prevent command injection."""
    if not input_str:
        return ""
    
    # Remove dangerous characters for shell commands
    # Allow alphanumeric, spaces, dots, dashes, underscores, and basic regex chars
    sanitized = re.sub(r'[^a-zA-Z0-9\s._\-\[\]\(\)\+\*\?\|\^$\\]', '', input_str)
    
    # Limit length
    return sanitized[:500]

def validate_file_pattern(pattern: str) -> bool:
    """Validate file pattern to prevent dangerous glob patterns."""
    if not pattern:
        return False
    
    # Basic validation for safe file patterns
    safe_pattern = re.match(r'^[\w\*\?\.\-/]+$', pattern)
    return bool(safe_pattern) and len(pattern) < 100

@app.list_resources()
async def handle_list_resources() -> List[Resource]:
    """List available source code resources."""
    resources = []
    
    # Add key directories as resources
    key_dirs = [
        "src/brave",
        "src/chrome", 
        "src/components",
        "src/content",
        "src/base"
    ]
    
    for dir_path in key_dirs:
        full_path = CURRENT_DIR / dir_path
        if full_path.exists():
            resources.append(Resource(
                uri=f"file://{full_path}",
                name=dir_path,
                description=f"Source code directory: {dir_path}",
                mimeType="text/plain"
            ))
    
    return resources

@app.read_resource()
async def handle_read_resource(uri: str) -> str:
    """Read a source code resource."""
    if not uri.startswith("file://"):
        raise ValueError("Only file:// URIs are supported")
    
    file_path = Path(uri[7:])  # Remove file:// prefix
    
    if not file_path.exists():
        raise FileNotFoundError(f"File not found: {file_path}")
    
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
        return content
    except UnicodeDecodeError:
        # Handle binary files
        return f"Binary file: {file_path}"

@app.list_tools()
async def handle_list_tools() -> List[Tool]:
    """List available tools for source code navigation."""
    return [
        Tool(
            name="search_files",
            description="Search for files by name pattern in the codebase",
            inputSchema={
                "type": "object",
                "properties": {
                    "pattern": {
                        "type": "string",
                        "description": "File name pattern to search for (supports glob patterns)"
                    },
                    "directory": {
                        "type": "string", 
                        "description": "Directory to search in (default: entire codebase)",
                        "default": "."
                    }
                },
                "required": ["pattern"]
            }
        ),
        Tool(
            name="search_code",
            description="Search for code patterns within files",
            inputSchema={
                "type": "object",
                "properties": {
                    "query": {
                        "type": "string",
                        "description": "Code pattern to search for"
                    },
                    "file_pattern": {
                        "type": "string",
                        "description": "File pattern to limit search (e.g., '*.cc', '*.h')",
                        "default": "*"
                    },
                    "directory": {
                        "type": "string",
                        "description": "Directory to search in",
                        "default": "."
                    }
                },
                "required": ["query"]
            }
        ),
        Tool(
            name="get_file_info",
            description="Get detailed information about a specific file",
            inputSchema={
                "type": "object",
                "properties": {
                    "file_path": {
                        "type": "string",
                        "description": "Path to the file to analyze"
                    }
                },
                "required": ["file_path"]
            }
        ),
        Tool(
            name="find_brave_components",
            description="Find Brave-specific components and their relationships to Chromium",
            inputSchema={
                "type": "object",
                "properties": {
                    "component_name": {
                        "type": "string",
                        "description": "Name of Brave component to search for",
                        "default": ""
                    }
                }
            }
        )
    ]

@app.call_tool()
async def handle_call_tool(name: str, arguments: Dict[str, Any]) -> List[TextContent]:
    """Handle tool calls for source code operations."""
    
    if name == "search_files":
        pattern = arguments["pattern"]
        directory = arguments.get("directory", ".")
        
        # Security validation
        if not validate_path(directory):
            return [TextContent(type="text", text="Error: Invalid or unsafe directory path")]
        
        if not validate_file_pattern(pattern):
            return [TextContent(type="text", text="Error: Invalid file pattern")]
        
        try:
            # Sanitize inputs for shell command
            safe_directory = shlex.quote(directory)
            safe_pattern = shlex.quote(pattern)
            
            # Use find command for file search with size limit
            cmd = ["find", safe_directory, "-name", safe_pattern, "-type", "f", "-size", f"-{MAX_FILE_SIZE}c"]
            result = subprocess.run(cmd, capture_output=True, text=True, cwd=CURRENT_DIR, timeout=30)
            
            if result.returncode == 0:
                files = result.stdout.strip().split('\n') if result.stdout.strip() else []
                # Limit results for performance
                files = files[:MAX_SEARCH_RESULTS]
                content = f"Found {len(files)} files matching '{pattern}':\n\n"
                for file in files:
                    content += f"- {file}\n"
                if len(files) == MAX_SEARCH_RESULTS:
                    content += f"\n... (limited to {MAX_SEARCH_RESULTS} results)"
            else:
                content = f"No files found matching '{pattern}'"
                
        except subprocess.TimeoutExpired:
            content = "Error: Search operation timed out"
        except Exception as e:
            content = f"Error: {str(e)}"
            
        return [TextContent(type="text", text=content)]
    
    elif name == "search_code":
        query = arguments["query"]
        file_pattern = arguments.get("file_pattern", "*")
        directory = arguments.get("directory", ".")
        
        # Security validation
        if not validate_path(directory):
            return [TextContent(type="text", text="Error: Invalid or unsafe directory path")]
        
        if not validate_file_pattern(file_pattern):
            return [TextContent(type="text", text="Error: Invalid file pattern")]
        
        # Sanitize search query
        safe_query = sanitize_search_input(query)
        if not safe_query:
            return [TextContent(type="text", text="Error: Invalid search query")]
        
        try:
            # Sanitize inputs for shell command
            safe_directory = shlex.quote(directory)
            safe_pattern = shlex.quote(file_pattern)
            safe_query_quoted = shlex.quote(safe_query)
            
            # Use ripgrep if available, otherwise grep with limits
            try:
                if subprocess.run(["which", "rg"], capture_output=True, timeout=5).returncode == 0:
                    cmd = ["rg", "--max-count", str(MAX_SEARCH_RESULTS), "-n", safe_query_quoted, safe_directory, "--glob", safe_pattern]
                else:
                    cmd = ["grep", "-r", "-n", "--include", safe_pattern, "--max-count", str(MAX_SEARCH_RESULTS), safe_query_quoted, safe_directory]
            except subprocess.TimeoutExpired:
                # Fall back to grep if which command times out
                cmd = ["grep", "-r", "-n", "--include", safe_pattern, "--max-count", str(MAX_SEARCH_RESULTS), safe_query_quoted, safe_directory]
                
            result = subprocess.run(cmd, capture_output=True, text=True, cwd=CURRENT_DIR, timeout=60)
            
            if result.returncode == 0:
                # Limit output size
                output = result.stdout[:50000]  # Limit to 50KB
                if len(result.stdout) > 50000:
                    output += "\n... (output truncated)"
                content = f"Code search results for '{safe_query}':\n\n{output}"
            else:
                content = f"No matches found for '{safe_query}'"
                
        except subprocess.TimeoutExpired:
            content = "Error: Search operation timed out"
        except Exception as e:
            content = f"Error: {str(e)}"
            
        return [TextContent(type="text", text=content)]
    
    elif name == "get_file_info":
        file_path = arguments["file_path"]
        
        # Security validation
        if not validate_path(file_path):
            return [TextContent(type="text", text="Error: Invalid or unsafe file path")]
        
        try:
            full_path = (CURRENT_DIR / file_path).resolve()
            
            # Double-check the resolved path is still safe
            if not validate_path(str(full_path)):
                return [TextContent(type="text", text="Error: Resolved path is outside allowed directories")]
            
            if not full_path.exists():
                content = f"File not found: {file_path}"
            else:
                # Get file stats
                stat = full_path.stat()
                size = stat.st_size
                
                # Check file size limit
                if size > MAX_FILE_SIZE:
                    content = f"File: {file_path}\n"
                    content += f"Size: {size} bytes (too large to read)\n"
                    content += "File exceeds maximum readable size limit"
                    return [TextContent(type="text", text=content)]
                
                # Check file extension is allowed
                suffix = full_path.suffix.lower()
                is_allowed = suffix in ALLOWED_FILE_EXTENSIONS
                is_source = suffix in {'.cc', '.cpp', '.c', '.h', '.hpp', '.py', '.js', '.ts', '.java'}
                
                content = f"File: {file_path}\n"
                content += f"Size: {size} bytes\n"
                content += f"Type: {'Source code' if is_source else 'Other'}\n"
                
                if is_allowed and is_source and size < MAX_FILE_SIZE:
                    try:
                        with open(full_path, 'r', encoding='utf-8', errors='ignore') as f:
                            lines = f.readlines()[:100]  # Limit to first 100 lines
                        content += f"Lines: {len(lines)} (showing first {min(20, len(lines))})\n\n"
                        content += "Preview:\n"
                        content += "```\n"
                        content += "".join(lines[:20])
                        if len(lines) > 20:
                            content += "\n... (truncated)"
                        content += "```"
                    except (UnicodeDecodeError, PermissionError):
                        content += "Cannot read file content (binary or permission denied)"
                elif not is_allowed:
                    content += "File type not allowed for reading"
                        
        except Exception as e:
            content = f"Error: {str(e)}"
            
        return [TextContent(type="text", text=content)]
    
    elif name == "find_brave_components":
        component_name = arguments.get("component_name", "")
        
        # Sanitize component name input
        if component_name:
            safe_component_name = sanitize_search_input(component_name)
            if not safe_component_name:
                return [TextContent(type="text", text="Error: Invalid component name")]
        else:
            safe_component_name = ""
        
        try:
            # Search for Brave-specific directories and files
            brave_paths = []
            
            # Search in brave/ directory with security constraints
            brave_dir = CURRENT_DIR / "src" / "brave"
            if brave_dir.exists():
                if safe_component_name:
                    safe_pattern = shlex.quote(f"*{safe_component_name}*")
                    cmd = ["find", str(brave_dir), "-name", safe_pattern, "-type", "d", "-maxdepth", "5"]
                else:
                    cmd = ["find", str(brave_dir), "-maxdepth", "2", "-type", "d"]
                    
                result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
                if result.returncode == 0:
                    paths = result.stdout.strip().split('\n') if result.stdout.strip() else []
                    # Validate all returned paths
                    for path in paths:
                        if path and validate_path(path):
                            brave_paths.append(path)
            
            content = f"Brave components"
            if safe_component_name:
                content += f" matching '{safe_component_name}'"
            content += ":\n\n"
            
            # Limit and validate results
            for path in brave_paths[:MAX_SEARCH_RESULTS//5]:  # Smaller limit for directory searches
                if path:
                    try:
                        rel_path = Path(path).relative_to(CURRENT_DIR)
                        content += f"- {rel_path}\n"
                    except ValueError:
                        # Skip paths that can't be made relative (security)
                        continue
                        
        except subprocess.TimeoutExpired:
            content = "Error: Component search timed out"
        except Exception as e:
            content = f"Error: {str(e)}"
            
        return [TextContent(type="text", text=content)]
    
    else:
        raise ValueError(f"Unknown tool: {name}")

async def main():
    """Main entry point for the MCP server."""
    async with stdio_server() as (read_stream, write_stream):
        await app.run(
            read_stream,
            write_stream,
            InitializationOptions(
                server_name="brave-source-access",
                server_version="1.0.0",
                capabilities=app.get_capabilities(
                    notification_options=NotificationOptions(),
                    experimental_capabilities={},
                ),
            ),
        )

if __name__ == "__main__":
    asyncio.run(main())