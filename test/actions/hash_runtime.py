import sys
import os
import subprocess
import hashlib
import json
from datetime import datetime
from glob import glob 

def hashStr (str):
  hash_sha256 = hashlib.sha256()
  hash_sha256.update(str.encode("utf-8"))
  return hash_sha256.hexdigest()

def hashFolder(file_path):
  pat = os.path.join(file_path, "**/*")
  for file in glob(pat):
    yield from hashFile(file)

def hashFile(file_path): 
  if os.path.isdir(file_path):
    yield from hashFolder(file_path)
  else:
    with open(file_path, "rb") as f:
      hash_sha256 = hashlib.sha256()
      for chunk in iter(lambda: f.read(4096), b""):
        hash_sha256.update(chunk)
    yield (file_path, hash_sha256.hexdigest())


def hashFiles(files): 
  for file in files:
    yield from hashFile(file)


# Only create a file if the content has changed.
# This ensures that dependees are not rebuilt unecessarily
def writeIfChanged(filePath, newContent):
  with open(filePath, "w+") as f:
    content = f.read()
    changed = content != newContent
    if changed:
      f.seek(0)
      f.write(newContent)   

# TODO: use argparser and make things more generic
def runtimeDeps(exePath, extraFiles):
  runtimeDeps = exePath + '.runtime_deps'
  jsonPath = exePath+".hash.json"
  depPath = jsonPath + ".d"

  with open(runtimeDeps, 'r') as f:
    deps = sorted(set(f.read().strip().split('\n') + extraFiles))

  files = list(hashFiles(deps))
  combinedHash = hashStr(",".join(map(lambda x: ":".join(x)  ,files)))

  meta = {
    "hash": combinedHash,
    "files": files
  }
  content = json.dumps(meta, indent=2)
  writeIfChanged(jsonPath, content)

  hashedDeps = list(map(lambda x: x[0].replace(" ", "\\ "), files))
  depStr = " ".join(hashedDeps).strip()
  content = f"{jsonPath} : {depStr}".replace(os.getcwd()+"/", "")
  writeIfChanged(depPath, content)


def main():
  exe = sys.argv[1]
  extraFiles = sys.argv[1:]
  runtimeDeps(exe, extraFiles)

if __name__ == '__main__':
  main()