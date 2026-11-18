# Brave checkout image

`Dockerfile` and `build_brave_image.py` build a reusable Linux x64 image with
Brave and dependencies for macOS x64, Windows x64, Linux x64, and Android arm64.

For prerequisites, credentials, Docker selection, image tags, and recovery
instructions, run from the brave-core checkout:

```sh
python3 tools/docker/build_brave_image.py --help
```

Run the tooling tests without Docker or remote build credentials:

```sh
python3 -m unittest discover -s tools/docker -p 'test_*.py'
```
