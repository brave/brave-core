# Copyright (c) 2026 The Brave Authors. All rights reserved.
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this file,
# You can obtain one at https://mozilla.org/MPL/2.0/.
"""Recipe configuration meta-DSL (a faithful, trimmed port of chrome-infra's
`recipe_engine/config.py`).

This module is, essentially, a DSL for writing composable configurations. You
start by defining a *schema* describing how your configuration blobs are
structured and what data they may hold. For example:

    def FakeSchema(main_val=True, mode='Happy'):
        return ConfigGroup(
            config_group=ConfigGroup(
                item_a=Single(int),
                item_b=Dict(),
            ),
            extra_setting=Set(str),
            MAIN_DETERMINANT=Static(main_val),
            CONFIG_MODE=Static(mode),
        )

In short, a schema is a callable which takes zero or more arguments (with sane
defaults) and returns a `ConfigGroup`.

Every type used in a schema derives from `ConfigBase`: a fixed-type container
that impersonates the data type it stores (so you manipulate config objects like
plain Python data) while also enforcing type checking (so blobs render cleanly
to JSON).

From a schema you create a configuration context:

    config_ctx = config_item_context(FakeSchema)

`config_ctx` is a decorator you use to define composable *config items*:

    @config_ctx()
    def cool(c):
        if c.CONFIG_MODE == 'Happy':
            c.config_group.item_a = 100
        else:
            c.config_group.item_a = -100

    @config_ctx()
    def gnarly(c):
        c.extra_setting.add('gnarly!')

    @config_ctx(includes=('cool', 'gnarly'))
    def combo(c):
        if c.MAIN_DETERMINANT:
            c.config_group.item_b['nickname'] = 'purple'
        else:
            c.config_group.item_b['nickname'] = 'sad times'

Calling `combo()` returns a configuration object whose schema is `FakeSchema`
and whose data is the accumulation of `cool()`, `gnarly()`, and `combo()`. You
can keep manipulating it, read its data, or render it to JSON via
`as_jsonish()`.

Differences from the upstream module (all cosmetic, none behavioural for the
config DSL itself):
  * The protobuf `schema_proto()` doc-export methods are omitted (the engine
    has no doc tooling).
  * The `config_types.Path` carve-out in `Static` is omitted; values are always
    hashed to enforce immutability (`pathlib.Path` is hashable).
  * `ConfigGroupSchema`/`ReturnSchema` (the separate recipe *return*-value
    feature) are omitted.
  * Python 2 shims (`basestring`, `from builtins import object`) are dropped.
"""

from __future__ import annotations

import collections.abc
import functools
import types


class BadConf(Exception):
    """Raised for any apply-time configuration violation.

    Double application of an item, an unfulfilled `deps` group, a `group`
    mutual-exclusion conflict, or a failing `includes` all surface as `BadConf`.
    """


def typeAssert(obj, typearg):
    """Assert `obj` is an instance of `typearg`, raising `TypeError` if not."""
    if not isinstance(obj, typearg):
        raise TypeError('Expected %r to be of type %r' % (obj, typearg))


class ConfigContext:
    """A configuration context for a recipe module.

    Holds the configuration schema and a registry of config items, and acts as
    the `config_ctx` decorator that populates that registry. A recipe module
    defines at most one such context (in its `config.py`).
    """

    def __init__(self, CONFIG_SCHEMA):
        self.CONFIG_ITEMS = {}
        self.MUTEX_GROUPS = {}
        self.CONFIG_SCHEMA = CONFIG_SCHEMA
        self.ROOT_CONFIG_ITEM = None

    def __call__(self, group=None, includes=None, deps=None, is_root=False):
        """Decorator for functions which modify a config blob.

        Args:
            group(str): Members of a group are mutually exclusive on the same
                config blob -- only one item of a given group may be applied.
            includes(iterable(str)): Config items to run against the blob before
                the decorated function's body. An include already applied to the
                blob is skipped.
            deps(iterable(str)): Group names which must already be satisfied on
                the blob (some member applied with `final=True`) before this item
                may apply; otherwise `BadConf`.
            is_root(bool): Marks this the single 'basis' item, implicitly applied
                before every other item. At most one root per context.

        Returns:
            A decorated version of the function (see `inner`).
        """

        def decorator(f):
            name = f.__name__

            @functools.wraps(f)
            def inner(config=None, final=True, optional=False, **kwargs):
                """Apply this config item to a config blob and return the blob.

                Args:
                    config: The config blob to manipulate. A fresh blob is made
                        from the schema when None. The item mutates it in place.
                    final(bool): When True (default), records this item's
                        application on the blob, preventing re-application and
                        allowing it to satisfy later `deps`. Apply with
                        `final=False` to seed defaults a later item may override.
                    optional(bool): When True, a double application returns the
                        blob unchanged instead of raising `BadConf`.
                    **kwargs: Passed through to the decorated function.

                Returns the (mutated) config blob; the body's return value is
                ignored (and asserted to be None).
                """
                if config is None:
                    config = self.CONFIG_SCHEMA()
                assert isinstance(config, ConfigGroup)
                inclusions = config._inclusions  # pylint: disable=protected-access

                # Auto-apply the root item first, unless we are the root or it
                # has already run on this blob.
                if (self.ROOT_CONFIG_ITEM and not inner.IS_ROOT
                        and self.ROOT_CONFIG_ITEM.__name__ not in inclusions):
                    self.ROOT_CONFIG_ITEM(config)

                if name in inclusions:
                    if optional:
                        return config
                    raise BadConf(
                        'config_ctx "%s" is already in this config "%s"' %
                        (name, config.as_jsonish(include_hidden=True)))
                if final:
                    inclusions.add(name)

                for include in includes or []:
                    if include in inclusions:
                        continue
                    try:
                        self.CONFIG_ITEMS[include](config)
                    except BadConf as e:
                        raise BadConf('config "%s" includes "%s", but [%s]' %
                                      (name, include, e)) from e

                # deps are group names; every group must already be represented.
                for dep_group in deps or []:
                    if not inclusions & self.MUTEX_GROUPS[dep_group]:
                        raise BadConf(
                            'dep group "%s" is unfulfilled for "%s"' %
                            (dep_group, name))

                if group:
                    overlap = inclusions & self.MUTEX_GROUPS[group]
                    overlap.discard(name)
                    if overlap:
                        raise BadConf(
                            '"%s" is a member of group "%s", but %s already ran'
                            % (name, group, tuple(overlap)))

                ret = f(config, **kwargs)
                assert ret is None, 'Got return value (%s) from "%s"?' % (ret,
                                                                          name)

                return config

            inner.WRAPPED = f
            inner.INCLUDES = includes or []

            assert name not in self.CONFIG_ITEMS, (
                '%s is already in CONFIG_ITEMS' % name)
            self.CONFIG_ITEMS[name] = inner
            if group:
                self.MUTEX_GROUPS.setdefault(group, set()).add(name)
            inner.IS_ROOT = is_root
            if is_root:
                assert not self.ROOT_CONFIG_ITEM, (
                    'may only have one root config_ctx!')
                self.ROOT_CONFIG_ITEM = inner
            return inner

        return decorator


def config_item_context(CONFIG_SCHEMA):
    """Create a configuration context.

    Args:
        CONFIG_SCHEMA: A callable taking zero or more (defaulted) arguments and
            returning a `ConfigGroup`. It defines the schema for all config
            blobs manipulated in this context.

    Returns:
        A `ConfigContext` (the `config_ctx` decorator) for this context.
    """
    return ConfigContext(CONFIG_SCHEMA)


class AutoHide:
    """Sentinel `hidden` mode: render a field only when not at its default."""


AutoHide = AutoHide()


class ConfigBase:
    """The root interface for all config schema types."""

    # Declared at class scope so static analysis sees them; each instance
    # assigns its own via object.__setattr__ in __init__ (which bypasses the
    # subclass __setattr__ overrides, e.g. ConfigGroup's).
    _hidden_mode: object
    _inclusions: set

    def __init__(self, hidden=AutoHide):
        """
        Args:
            hidden:
                True: excluded from `ConfigGroup.as_jsonish()` output (you keep
                    full read/write access otherwise).
                False: always rendered by `as_jsonish()`.
                AutoHide: rendered only when `_is_default()` is False.
        """
        # Bypass subclasses that override __setattr__.
        object.__setattr__(self, '_hidden_mode', hidden)
        object.__setattr__(self, '_inclusions', set())

    def get_val(self):
        """Return the native value of this config object."""
        return self

    def set_val(self, val):
        """Reset this config object's value from `val`."""
        raise NotImplementedError

    def reset(self):
        """Reset this config object to its initial state."""
        raise NotImplementedError

    def as_jsonish(self, include_hidden=False):
        """Return this config object's value as simple JSON-compatible types."""
        raise NotImplementedError

    def complete(self):
        """Return True iff this blob is fully viable (required fields set)."""
        raise NotImplementedError

    def _is_default(self):
        """Return True iff this blob is at its default value."""
        raise NotImplementedError

    @property
    def _hidden(self):
        """Return True iff this blob is hidden from `as_jsonish()`."""
        if self._hidden_mode is AutoHide:
            return self._is_default()
        return self._hidden_mode


class ConfigGroup(ConfigBase):
    """Provides hierarchy to a configuration schema.

    Example:
        config_blob = ConfigGroup(
            some_item=Single(str),
            group=ConfigGroup(
                numbahs=Set(int),
            ),
        )
        config_blob.some_item = 'hello'
        config_blob.group.numbahs.update(range(10))
    """

    def __init__(self, hidden=AutoHide, **type_map):
        """Expects `type_map` to be a `{python_name -> ConfigBase}` mapping."""
        super().__init__(hidden)
        assert type_map, 'A ConfigGroup with no type_map is meaningless.'

        object.__setattr__(self, '_type_map', type_map)
        for name, typeval in self._type_map.items():
            typeAssert(typeval, ConfigBase)
            object.__setattr__(self, name, typeval)

    def __getattribute__(self, name):
        obj = object.__getattribute__(self, name)
        if isinstance(obj, ConfigBase):
            return obj.get_val()
        return obj

    def __setattr__(self, name, val):
        obj = object.__getattribute__(self, name)
        typeAssert(obj, ConfigBase)
        obj.set_val(val)

    def __delattr__(self, name):
        obj = object.__getattribute__(self, name)
        typeAssert(obj, ConfigBase)
        obj.reset()

    def set_val(self, val):
        if isinstance(val, ConfigBase):
            val = val.as_jsonish(include_hidden=True)
        typeAssert(val, collections.abc.Mapping)

        val = dict(val)  # Copied because we pop below.
        for name, config_obj in self._type_map.items():
            if name in val:
                try:
                    config_obj.set_val(val.pop(name))
                except Exception as e:
                    raise type(e)('While assigning key %r: %s' % (name, e))

        if val:
            raise TypeError('Got extra keys while setting ConfigGroup: %s' %
                            val)

    def as_jsonish(self, include_hidden=False):
        return {
            n: v.as_jsonish(include_hidden)
            for n, v in self._type_map.items()
            if include_hidden or not v._hidden  # pylint: disable=protected-access
        }

    def reset(self):
        for v in self._type_map.values():
            v.reset()

    def complete(self):
        return all(v.complete() for v in self._type_map.values())

    def _is_default(self):
        # pylint: disable=protected-access
        return all(v._is_default() for v in self._type_map.values())


class ConfigList(ConfigBase, collections.abc.MutableSequence):
    """Provides ordered repetition to a configuration schema.

    Example:
        config_blob = ConfigGroup(
            some_items=ConfigList(
                lambda: ConfigGroup(
                    herp=Single(int),
                    derp=Single(str),
                )
            )
        )
        config_blob.some_items.append({'herp': 1})
        config_blob.some_items[0].derp = 'bob'
    """

    def __init__(self, item_schema, hidden=AutoHide):
        """
        Args:
            item_schema: A function returning an instance of `ConfigGroup` -- the
                schema of each element.
        """
        super().__init__(hidden=hidden)
        typeAssert(item_schema, types.FunctionType)
        typeAssert(item_schema(), ConfigGroup)
        self.item_schema = item_schema
        self.data = []

    def __getitem__(self, index):
        return self.data.__getitem__(index)

    def __setitem__(self, index, value):
        datum = self.item_schema()
        datum.set_val(value)
        return self.data.__setitem__(index, datum)

    def __delitem__(self, index):
        return self.data.__delitem__(index)

    def __len__(self):
        return len(self.data)

    def insert(self, index, value):
        datum = self.item_schema()
        datum.set_val(value)
        return self.data.insert(index, datum)

    def add(self):
        self.append({})
        return self[-1]

    def reset(self):
        self.data = []

    def complete(self):
        return all(i.complete() for i in self.data)

    def set_val(self, val):
        if isinstance(val, ConfigList):
            val = val.as_jsonish(include_hidden=True)

        typeAssert(val, list)
        self.reset()
        for item in val:
            self.append(item)

    def as_jsonish(self, include_hidden=False):
        return [
            i.as_jsonish(include_hidden) for i in self.data
            if include_hidden or not i._hidden  # pylint: disable=protected-access
        ]

    def _is_default(self):
        # pylint: disable=protected-access
        return all(v._is_default() for v in self.data)


class Dict(ConfigBase, collections.abc.MutableMapping):
    """Provides a semi-homogeneous dict()-like configuration object."""

    def __init__(self,
                 item_fn=lambda i: i,
                 jsonish_fn=dict,
                 value_type=None,
                 hidden=AutoHide):
        """
        Args:
            item_fn: Renders (k, v) pairs to input items for `jsonish_fn`.
                Defaults to the identity function.
            jsonish_fn: Renders the list of `item_fn` outputs to a
                JSON-compatible type. Defaults to `dict`.
            value_type: A type used to constrain assigned values. When None any
                type is accepted.
            hidden: See `ConfigBase`.
        """
        super().__init__(hidden)
        self.value_type = value_type
        self.item_fn = item_fn
        self.jsonish_fn = jsonish_fn
        self.data = {}

    def __getitem__(self, k):
        return self.data.__getitem__(k)

    def __setitem__(self, k, v):
        if self.value_type:
            typeAssert(v, self.value_type)
        return self.data.__setitem__(k, v)

    def __delitem__(self, k):
        return self.data.__delitem__(k)

    def __iter__(self):
        return iter(self.data)

    def __len__(self):
        return len(self.data)

    def __repr__(self):
        return repr(self.data)

    def __str__(self):
        return str(self.data)

    def set_val(self, val):
        if isinstance(val, Dict):
            val = val.data
        typeAssert(val, collections.abc.Mapping)
        if self.value_type:
            for v in val.values():
                typeAssert(v, self.value_type)
        self.data = val

    def as_jsonish(self, _include_hidden=None):
        return self.jsonish_fn([
            self.item_fn(item)
            for item in sorted(self.data.items(), key=lambda x: x[0])
        ])

    def reset(self):
        self.data.clear()

    def complete(self):
        return True

    def _is_default(self):
        return not self.data


class List(ConfigBase, collections.abc.MutableSequence):
    """Provides a semi-homogeneous list()-like configuration object."""

    def __init__(self, inner_type, jsonish_fn=list, hidden=AutoHide):
        """
        Args:
            inner_type: The type of data in this list (e.g. str, int, ...). May
                be a tuple of types to allow more than one.
            jsonish_fn: Reduces the list to a JSON-compatible type. Defaults to
                `list`.
            hidden: See `ConfigBase`.
        """
        super().__init__(hidden)
        self.inner_type = inner_type
        self.jsonish_fn = jsonish_fn
        self.data = []

    def __getitem__(self, index):
        return self.data[index]

    def __setitem__(self, index, value):
        typeAssert(value, self.inner_type)
        self.data[index] = value

    def __delitem__(self, index):
        del self.data[index]

    def __len__(self):
        return len(self.data)

    def __radd__(self, other):
        if not isinstance(other, list):
            other = list(other)
        return other + self.data

    def insert(self, index, value):
        typeAssert(value, self.inner_type)
        self.data.insert(index, value)

    def set_val(self, val):
        for v in val:
            typeAssert(v, self.inner_type)
        self.data = list(val)

    def as_jsonish(self, _include_hidden=None):
        return self.jsonish_fn(self.data)

    def reset(self):
        self.data = []

    def complete(self):
        return True

    def _is_default(self):
        return not self.data


class Set(ConfigBase, collections.abc.MutableSet):
    """Provides a semi-homogeneous set()-like configuration object."""

    def __init__(self, inner_type, jsonish_fn=list, hidden=AutoHide):
        """
        Args:
            inner_type: The type of data in this set (e.g. str, int, ...). May be
                a tuple of types to allow more than one.
            jsonish_fn: Reduces the set to a JSON-compatible type. Defaults to
                `list`.
            hidden: See `ConfigBase`.
        """
        super().__init__(hidden)
        self.inner_type = inner_type
        self.jsonish_fn = jsonish_fn
        self.data = set()

    def __contains__(self, val):
        return val in self.data

    def __iter__(self):
        return iter(self.data)

    def __len__(self):
        return len(self.data)

    def add(self, value):
        typeAssert(value, self.inner_type)
        self.data.add(value)

    def update(self, values):
        for value in values:
            if value not in self:
                self.add(value)

    def discard(self, value):
        self.data.discard(value)

    def set_val(self, val):
        for v in val:
            typeAssert(v, self.inner_type)
        self.data = set(val)

    def as_jsonish(self, _include_hidden=None):
        return self.jsonish_fn(sorted(self.data))

    def reset(self):
        self.data = set()

    def complete(self):
        return True

    def _is_default(self):
        return not self.data


class Single(ConfigBase):
    """Provides a configuration object which holds a single 'simple' type."""

    def __init__(self,
                 inner_type,
                 jsonish_fn=lambda x: x,
                 empty_val=None,
                 required=True,
                 hidden=AutoHide):
        """
        Args:
            inner_type: The type of data held (e.g. str, int, ...). May be a
                tuple of types to allow more than one.
            jsonish_fn: Reduces the data to a JSON-compatible type. Defaults to
                the identity function.
            empty_val: The value used to initialize this object and on `reset()`.
            required(bool): True iff this item must hold a non-`empty_val` value
                for the blob to be `complete()`.
            hidden: See `ConfigBase`.
        """
        super().__init__(hidden)
        self.inner_type = inner_type
        self.jsonish_fn = jsonish_fn
        self.empty_val = empty_val
        self.data = empty_val
        self.required = required

    def get_val(self):
        return self.data

    def set_val(self, val):
        if isinstance(val, Single):
            val = val.data
        if val is not self.empty_val:
            typeAssert(val, self.inner_type)
        self.data = val

    def as_jsonish(self, _include_hidden=None):
        return self.jsonish_fn(self.data)

    def reset(self):
        self.data = self.empty_val

    def complete(self):
        return not self.required or self.data is not self.empty_val

    def _is_default(self):
        return self.data is self.empty_val


class Static(ConfigBase):
    """Holds a single, hidden, immutable data object.

    Very useful for holding the 'input' configuration values threaded in through
    the schema factory's arguments (the `CONFIG_VARS` a caller passes).
    """

    def __init__(self, value, hidden=AutoHide):
        super().__init__(hidden=hidden)
        # Hash the value to ensure it is immutable all the way down.
        hash(value)
        self.data = value

    def get_val(self):
        return self.data

    def set_val(self, val):
        raise TypeError('Cannot assign to a Static config member')

    def as_jsonish(self, _include_hidden=None):
        return self.data

    def reset(self):
        assert False

    def complete(self):
        return True

    def _is_default(self):
        return True


class Enum(ConfigBase):
    """Provides a configuration object which holds one of a fixed set of values.
    """

    def __init__(self, *values, **kwargs):
        """
        Args:
            values: The list of acceptable values.
            inner_type: The type of data held (e.g. str, int, ...). May be a
                tuple of types. Defaults to `str`.
            jsonish_fn: Reduces the data to a JSON-compatible type. Defaults to
                the identity function.
            required(bool): True iff this item must hold a value for the blob to
                be `complete()`.
            hidden: See `ConfigBase`.
        """
        super().__init__(kwargs.get('hidden', AutoHide))
        if not values:
            raise ValueError('Enumerations cannot be empty')
        self.values = values
        self.inner_type = kwargs.get('inner_type', str)
        self.jsonish_fn = kwargs.get('jsonish_fn', lambda x: x)
        self.data = None
        self.required = kwargs.get('required', True)

    def get_val(self):
        return self.data

    def set_val(self, val):
        if isinstance(val, Enum):
            val = val.data
        typeAssert(val, self.inner_type)
        if val not in self.values:
            raise ValueError('Expected %r to be one of %r' %
                             (val, ', '.join(self.values)))
        self.data = val

    def as_jsonish(self, _include_hidden=None):
        return self.jsonish_fn(self.data)

    def reset(self):
        self.data = None

    def complete(self):
        return not self.required or self.data is not None

    def _is_default(self):
        return self.data is None
