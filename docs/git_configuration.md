## Git Configuration

It is recommended to use this configuration in `.gitconfig` to ensure consistent whitespace rules, improve code readability, prevent unnecessary diffs, and maintain proper file formatting with a final newline.

```
[core]
    whitespace=fix,-indent-with-non-tab,trailing-space,cr-at-eol
```

- `fix`: Automatically fixes whitespace issues in patches (e.g., removes/trims trailing spaces, adds a carriage return at the end of a file if it's missing, etc.).
- `-indent-with-non-tab`: Prevents warnings about lines that are indented with spaces instead of tabs (essentially ignores this issue).
- `trailing-space`: Flags or fixes trailing spaces at the end of lines.
- `cr-at-eol`: Ensures that files end with a carriage return (newline) at the end of the last line.
