#!/usr/bin/python
#
# Clean up NSLocalizedString usage excluding files within "blacklisted_parent_directories"

import os
import re

blacklisted_parent_directories = ["ThirdParty", "Carthage", "fastlane", "L10nSnapshotTests", "l10n"]
frameworks = ["BraveShared", "Data", "Shared", "Storage"]

def pascal_case(string):
  # Convert fullstops, hyphens and underscores to spaces so that words are correctly pascal cased
  string = string.replace(".", " ")
  string = string.replace("-", " ")
  string = string.replace("_", " ")

  # Convert first letter of each word to uppercase
  string = re.sub(r'(^|\s)(\S)', lambda match: match.group(1) + match.group(2).upper(), string)

  # Strip punctuation
  string = re.sub(r'[^\w\s]', '', string)

  # Strip spaces
  string = string.replace(" ", "")

  return string

def replacement_string(key, value, tablename, comment, is_framework):
  # func NSLocalizedString(_ key: String,
  #                    tableName: String? = default,
  #                       bundle: Bundle = default,
  #                        value: String = default,
  #                      comment: String

  content = 'NSLocalizedString("' + pascal_case(key) + '"'

  if is_framework:
    content += ', tableName: "' + tablename + '"'

  content += ', value: "' + value + '"'

  content += ', comment: "' + comment + '")'

  return content

def parent_directory(path):
  norm_path = os.path.normpath(path)
  path_components = norm_path.split(os.sep)
  directory = path_components[0]
  return directory

def should_skip_path(path):
  directory = parent_directory(path)
  if directory in blacklisted_parent_directories:
    return True

  return False

for path, directories, files in os.walk("."):
  for file in files:
    if should_skip_path(path):
      continue

    if file.endswith(".swift"):
      framework = ""
      is_framework = False

      directory = parent_directory(path)
      if directory in frameworks:
        framework = directory
        is_framework = True

      if is_framework:
      	print "Processing " + path + "/" + file + " [Framework: " + framework + "]"
      else:
        print "Processing " + path + "/" + file

      quotedStringPattern = '"([^"]*)"'

      keyPattern = ' *' + quotedStringPattern + ' *'
      tableNamePattern = ',[ \t]*(.?)[ \t]*tableName:[ \t]*(.?)[ \t]*' + quotedStringPattern + '[ \t]*'
      valuePattern = ',[ \t]*(.?)[ \t]*value:[ \t]*(.?)[ \t]*' + quotedStringPattern + '[ \t]*'
      commentPattern = ',[ \t]*(.?)[ \t]*comment:[ \t]*(.?)[ \t]*' + quotedStringPattern + '[ \t]*'

      flags = re.MULTILINE | re.DOTALL

      with open(os.path.join(path, file), 'r') as source:
        content = source.read()

        # Where K = key
        #       T = tableName
        #       B = bundle
        #       V = value
        #       C = comment

        # KTVC
        pattern = 'NSLocalizedString\(' + keyPattern + tableNamePattern + valuePattern + commentPattern + '\)'
        replacement_pattern = lambda match: replacement_string(match.group(1), match.group(7), framework, match.group(10), is_framework)
        content = re.sub(pattern, replacement_pattern, content, flags = flags)

        # KVTC
        pattern = 'NSLocalizedString\(' + keyPattern + valuePattern + tableNamePattern + commentPattern + '\)'
        replacement_pattern = lambda match: replacement_string(match.group(1), match.group(4), framework, match.group(10), is_framework)
        content = re.sub(pattern, replacement_pattern, content, flags = flags)

        # KVT-
        pattern = 'NSLocalizedString\(' + keyPattern + valuePattern + tableNamePattern + '\)'
        replacement_pattern = lambda match: replacement_string(match.group(1), match.group(4), framework, "", is_framework)
        content = re.sub(pattern, replacement_pattern, content, flags = flags)

        # KV-C
        pattern = 'NSLocalizedString\(' + keyPattern + valuePattern + commentPattern + '\)'
        replacement_pattern = lambda match: replacement_string(match.group(1), match.group(4), framework, match.group(7), is_framework)
        content = re.sub(pattern, replacement_pattern, content, flags = flags)

        # KV--
        pattern = 'NSLocalizedString\(' + keyPattern + valuePattern + '\)'
        replacement_pattern = lambda match: replacement_string(match.group(1), match.group(4), framework, "", is_framework)
        content = re.sub(pattern, replacement_pattern, content, flags = flags)

        # K-TC
        pattern = 'NSLocalizedString\(' + keyPattern + tableNamePattern + commentPattern + '\)'
        replacement_pattern = lambda match: replacement_string(match.group(1), match.group(1), framework, match.group(7), is_framework)
        content = re.sub(pattern, replacement_pattern, content, flags = flags)

        # K-T-
        pattern = 'NSLocalizedString\(' + keyPattern + tableNamePattern + '\)'
        replacement_pattern = lambda match: replacement_string(match.group(1), match.group(1), framework, "", is_framework)
        content = re.sub(pattern, replacement_pattern, content, flags = flags)

        # K--C
        pattern = 'NSLocalizedString\(' + keyPattern + commentPattern + '\)'
        replacement_pattern = lambda match: replacement_string(match.group(1), match.group(1), framework, match.group(4), is_framework)
        content = re.sub(pattern, replacement_pattern, content, flags = flags)

        # K---
        pattern = 'NSLocalizedString\(' + keyPattern + '\)'
        replacement_pattern = lambda match: replacement_string(match.group(1), match.group(1), framework, "", is_framework)
        content = re.sub(pattern, replacement_pattern, content, flags = flags)

        with open(os.path.join(path, file), 'w') as source:
          source.write(content)
