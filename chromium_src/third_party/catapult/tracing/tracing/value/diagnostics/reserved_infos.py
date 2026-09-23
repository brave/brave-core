BRAVE_CHROME_VERSION = _Info('braveChromeVersion', 'GenericSet', str)
BRAVE_TAG = _Info('braveTag', 'GenericSet', str)
BRAVE_VARIATIONS_REVISIONS = _Info('braveVariationsRevisions', 'GenericSet',
                                   str)
BRAVE_JOB_NAME = _Info('braveJobName', 'GenericSet', str)
BRAVE_JOB_ID = _Info('braveJobId', 'GenericSet', str)
BRAVE_TRACE_PATH = _Info('braveTracePath', 'GenericSet', str)

# Rewrite _CACHED_INFO_TYPES
_CACHED_INFO_TYPES = _CreateCachedInfoTypes()
