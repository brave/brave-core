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
        
        try:
            # Use find command for file search
            cmd = ["find", directory, "-name", pattern, "-type", "f"]
            result = subprocess.run(cmd, capture_output=True, text=True, cwd=CURRENT_DIR)
            
            if result.returncode == 0:
                files = result.stdout.strip().split('\n') if result.stdout.strip() else []
                content = f"Found {len(files)} files matching '{pattern}':\n\n"
                for file in files[:50]:  # Limit to first 50 results
                    content += f"- {file}\n"
                if len(files) > 50:
                    content += f"\n... and {len(files) - 50} more files"
            else:
                content = f"Error searching for files: {result.stderr}"
                
        except Exception as e:
            content = f"Error: {str(e)}"
            
        return [TextContent(type="text", text=content)]
    
    elif name == "search_code":
        query = arguments["query"]
        file_pattern = arguments.get("file_pattern", "*")
        directory = arguments.get("directory", ".")
        
        try:
            # Use ripgrep if available, otherwise grep
            if subprocess.run(["which", "rg"], capture_output=True).returncode == 0:
                cmd = ["rg", "--type-add", f"custom:{file_pattern}", "-t", "custom", "-n", query, directory]
            else:
                cmd = ["grep", "-r", "-n", "--include", file_pattern, query, directory]
                
            result = subprocess.run(cmd, capture_output=True, text=True, cwd=CURRENT_DIR)
            
            if result.returncode == 0:
                content = f"Code search results for '{query}':\n\n{result.stdout}"
            else:
                content = f"No matches found for '{query}'"
                
        except Exception as e:
            content = f"Error: {str(e)}"
            
        return [TextContent(type="text", text=content)]
    
    elif name == "get_file_info":
        file_path = arguments["file_path"]
        full_path = CURRENT_DIR / file_path
        
        try:
            if not full_path.exists():
                content = f"File not found: {file_path}"
            else:
                # Get file stats
                stat = full_path.stat()
                size = stat.st_size
                
                # Try to determine if it's a source file
                suffix = full_path.suffix
                is_source = suffix in ['.cc', '.cpp', '.c', '.h', '.hpp', '.py', '.js', '.ts']
                
                content = f"File: {file_path}\n"
                content += f"Size: {size} bytes\n"
                content += f"Type: {'Source code' if is_source else 'Other'}\n"
                
                if is_source and size < 1000000:  # Only read if < 1MB
                    try:
                        with open(full_path, 'r', encoding='utf-8') as f:
                            lines = f.readlines()
                        content += f"Lines: {len(lines)}\n\n"
                        content += "First 20 lines:\n"
                        content += "```\n"
                        content += "".join(lines[:20])
                        content += "```"
                    except UnicodeDecodeError:
                        content += "Binary file - cannot display content"
                        
        except Exception as e:
            content = f"Error: {str(e)}"
            
        return [TextContent(type="text", text=content)]
    
    elif name == "find_brave_components":
        component_name = arguments.get("component_name", "")
        
        try:
            # Search for Brave-specific directories and files
            brave_paths = []
            
            # Search in brave/ directory
            brave_dir = CURRENT_DIR / "src" / "brave"
            if brave_dir.exists():
                if component_name:
                    cmd = ["find", str(brave_dir), "-name", f"*{component_name}*", "-type", "d"]
                else:
                    cmd = ["find", str(brave_dir), "-maxdepth", "2", "-type", "d"]
                    
                result = subprocess.run(cmd, capture_output=True, text=True)
                if result.returncode == 0:
                    brave_paths.extend(result.stdout.strip().split('\n'))
            
            content = f"Brave components"
            if component_name:
                content += f" matching '{component_name}'"
            content += ":\n\n"
            
            for path in brave_paths[:20]:  # Limit results
                if path:
                    rel_path = Path(path).relative_to(CURRENT_DIR)
                    content += f"- {rel_path}\n"
                    
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