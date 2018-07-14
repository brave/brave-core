#!/usr/bin/python

import os
import re

ignore_parent_directories = ["*test", "ThirdParty", "Carthage", "fastlane", "L10nSnapshotTests", "l10n"]
frameworks = ["BraveShared", "Data", "Shared", "Storage"]

def pascal_case(string):
  string = re.sub(r'[\s]+(?P<first>[a-z])', lambda m: m.group('first').upper(), string)
  string = re.sub(r'[^\w\s]', '', string)
  return string

def replacement_string(key, value, tablename, comment, is_framework):
  content = 'NSLocalizedString("' + pascal_case(key) + '", value: "' + value + '"'
  if is_framework:
    content += ', tableName: "' + tablename + '"'
  content += ', comment: "' + comment + '")'
  return content

def parent_directory(path):
  norm_path = os.path.normpath(path)
  path_components = norm_path.split(os.sep)
  directory = path_components[0]
  return directory

def should_skip_path(path):
  directory = parent_directory(path)
  if directory in ignore_parent_directories:
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

      with open(os.path.join(path, file), 'r') as source:
        content = source.read()
        
        regex = 'NSLocalizedString\("([^"]*)"\)'
        content = re.sub(regex, lambda match: replacement_string(match.group(1), match.group(1), "", "", is_framework), content)

        regex = 'NSLocalizedString\("([^"]*)", value: "([^"]*)"\)'
        content = re.sub(regex, lambda match: replacement_string(match.group(1), match.group(2), "", "", is_framework), content)

        regex = 'NSLocalizedString\("([^"]*)", tableName: "([^"]*)"\)'
        content = re.sub(regex, lambda match: replacement_string(match.group(1), match.group(1), match.group(2), "", is_framework), content)

        regex = 'NSLocalizedString\("([^"]*)", tableName: "([^"]*)", comment: "([^"]*)"\)'
        content = re.sub(regex, lambda match: replacement_string(match.group(1), match.group(1), match.group(2), match.group(3), is_framework), content)

        regex = 'NSLocalizedString\("([^"]*)", comment: "([^"]*)"\)'
        content = re.sub(regex, lambda match: replacement_string(match.group(1), match.group(1), "", match.group(2), is_framework), content)

        regex = 'NSLocalizedString\("([^"]*)", value: "([^"]*)", comment: "([^"]*)"\)'
        content = re.sub(regex, lambda match: replacement_string(match.group(1), match.group(2), "", match.group(3), is_framework), content)

        regex = 'NSLocalizedString\("([^"]*)", value: "([^"]*)", tableName: "([^"]*)"\)'
        content = re.sub(regex, lambda match: replacement_string(match.group(1), match.group(1), match.group(2), match.group(3), is_framework), content)

        regex = 'NSLocalizedString\("([^"]*)", value: "([^"]*)", tableName: "([^"]*)", comment: "([^"]*)"\)'
        content = re.sub(regex, lambda match: replacement_string(match.group(1), match.group(2), framework, match.group(4), is_framework), content)

        with open(os.path.join(path, file), 'w') as source:
          source.write(content)
