from pathlib import Path
import sys


def human_readable_size(size, decimal_places=2):
    for unit in ['B', 'KiB', 'MiB', 'GiB', 'TiB', 'PiB']:
        if size < 1024.0 or unit == 'PiB':
            break
        size /= 1024.0
    return f"{size:.{decimal_places}f} {unit}"


def main():
    args = sys.argv[1:]
    root_directory = Path(args[0])
    size = sum(f.stat().st_size for f in root_directory.glob('**/*') if f.is_file())
    print(human_readable_size(size, 3))


if __name__ == '__main__':
    sys.exit(main())
