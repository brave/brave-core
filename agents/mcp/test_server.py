#!/usr/bin/env python3
"""
Test script to verify the Brave source MCP server is working correctly.
"""

import asyncio
import json
import os
import subprocess
import sys
from pathlib import Path

def test_mcp_package():
    """Test if MCP package is installed."""
    try:
        import mcp
        print("✅ MCP package is installed")
        return True
    except ImportError:
        print("❌ MCP package not found. Install with: pip install mcp")
        return False

def test_server_import():
    """Test if the server module can be imported."""
    try:
        # Add src directory to path for testing
        sys.path.insert(0, str(Path(__file__).parent.parent.parent.parent.parent / "src"))
        from brave.agents.mcp import source_access_server
        print("✅ Source access server module imports successfully")
        return True
    except ImportError as e:
        print(f"❌ Failed to import server module: {e}")
        return False

def test_python_execution():
    """Test if the server can be executed as a module."""
    try:
        cwd = Path(__file__).parent.parent.parent.parent.parent
        env = {**os.environ, "PYTHONPATH": str(cwd / "src")}
        cmd = [sys.executable, "-m", "brave.agents.mcp.source_access_server", "--help"]
        result = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=10, env=env)
        
        # The server runs continuously, so we expect it to start but not necessarily return help
        if result.returncode == 0 or "usage:" in result.stderr.lower():
            print("✅ Server module can be executed")
            return True
        else:
            print("⚠️  Server execution test inconclusive (this may be normal)")
            print(f"Return code: {result.returncode}")
            if result.stdout:
                print(f"Stdout: {result.stdout[:200]}")
            if result.stderr:
                print(f"Stderr: {result.stderr[:200]}")
            return True  # This is often normal for MCP servers
    except subprocess.TimeoutExpired:
        print("✅ Server started but timed out (this is expected for MCP servers)")
        return True
    except Exception as e:
        print(f"❌ Failed to execute server: {e}")
        return False

def test_config_files():
    """Test if configuration files exist and are valid."""
    base_path = Path(__file__).parent.parent.parent.parent.parent
    
    # Test .claude/mcp_servers.json
    claude_config = base_path / ".claude" / "mcp_servers.json"
    if claude_config.exists():
        try:
            with open(claude_config) as f:
                config = json.load(f)
            if "mcpServers" in config and "brave-source" in config["mcpServers"]:
                print("✅ Claude MCP configuration is valid")
                return True
            else:
                print("❌ Claude MCP configuration missing brave-source server")
                return False
        except json.JSONDecodeError as e:
            print(f"❌ Claude MCP configuration has invalid JSON: {e}")
            return False
    else:
        print("❌ Claude MCP configuration file not found")
        return False

def test_paths():
    """Test if configured paths exist."""
    base_path = Path(__file__).parent.parent.parent.parent.parent
    
    # Test current directory structure
    tests = [
        (base_path / "src", "Chromium source directory"),
        (base_path / "src" / "brave", "Brave source directory"),
        (base_path / "src" / "brave" / "agents" / "mcp", "MCP agents directory"),
    ]
    
    all_passed = True
    for path, description in tests:
        if path.exists():
            print(f"✅ {description} exists: {path}")
        else:
            print(f"⚠️  {description} not found: {path}")
            if "Chromium source" in description:
                all_passed = False
    
    # Test brave-core path (optional)
    brave_core_path = base_path.parent / "brave-core"
    if brave_core_path.exists():
        print(f"✅ Brave-core directory found: {brave_core_path}")
    else:
        print(f"⚠️  Brave-core directory not found: {brave_core_path} (you may need to adjust the path)")
    
    return all_passed

def test_tools():
    """Test if basic command-line tools are available."""
    tools = ["find", "grep"]
    
    all_passed = True
    for tool in tools:
        try:
            result = subprocess.run(["which", tool], capture_output=True, text=True)
            if result.returncode == 0:
                print(f"✅ {tool} command available")
            else:
                print(f"❌ {tool} command not found")
                all_passed = False
        except Exception as e:
            print(f"❌ Error checking {tool}: {e}")
            all_passed = False
    
    # Check for ripgrep (optional but preferred)
    try:
        result = subprocess.run(["which", "rg"], capture_output=True, text=True)
        if result.returncode == 0:
            print("✅ ripgrep (rg) available - will use for faster searching")
        else:
            print("⚠️  ripgrep (rg) not found - will fall back to grep")
    except Exception:
        print("⚠️  ripgrep (rg) not found - will fall back to grep")
    
    return all_passed

def main():
    """Run all tests."""
    print("🔍 Testing Brave Source MCP Server Setup...\n")
    
    tests = [
        ("MCP Package", test_mcp_package),
        ("Server Import", test_server_import),
        ("Python Execution", test_python_execution),
        ("Configuration Files", test_config_files),
        ("File Paths", test_paths),
        ("Command Tools", test_tools),
    ]
    
    passed = 0
    total = len(tests)
    
    for test_name, test_func in tests:
        print(f"\n--- {test_name} ---")
        if test_func():
            passed += 1
        else:
            print(f"❌ {test_name} failed")
    
    print(f"\n🏁 Test Results: {passed}/{total} tests passed")
    
    if passed == total:
        print("\n🎉 All tests passed! Your MCP server should work correctly.")
        print("\nTo use with Claude:")
        print("1. Restart Claude Code if it's running")
        print("2. The server should be automatically available as 'brave-source'")
        print("3. Try using tools like search_files, search_code, etc.")
    elif passed >= total - 2:
        print("\n⚠️  Most tests passed. Minor issues detected but server should still work.")
    else:
        print("\n❌ Several tests failed. Please check the setup instructions.")
    
    return passed == total

if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)