# MLIR Python Bindings

Current status: Under development and not enabled by default

## Building

### Pre-requisites

* A relatively recent Python3 installation
* [`pybind11`](https://github.com/pybind/pybind11) must be installed and able to
  be located by CMake (auto-detected if installed via
  `python -m pip install pybind11`). Note: minimum version required: :2.6.0.

### CMake variables

* **`MLIR_BINDINGS_PYTHON_ENABLED`**`:BOOL`

  Enables building the Python bindings. Defaults to `OFF`.

* **`Python3_EXECUTABLE`**:`STRING`

  Specifies the `python` executable used for the LLVM build, including for
  determining header/link flags for the Python bindings. On systems with
  multiple Python implementations, setting this explicitly to the preferred
  `python3` executable is strongly recommended.

* **`MLIR_PYTHON_BINDINGS_VERSION_LOCKED`**`:BOOL`

  Links the native extension against the Python runtime library, which is
  optional on some platforms. While setting this to `OFF` can yield some greater
  deployment flexibility, linking in this way allows the linker to report
  compile time errors for unresolved symbols on all platforms, which makes for a
  smoother development workflow. Defaults to `ON`.

### Recommended development practices

It is recommended to use a python virtual environment. Many ways exist for this,
but the following is the simplest:

```shell
# Make sure your 'python' is what you expect. Note that on multi-python
# systems, this may have a version suffix, and on many Linuxes and MacOS where
# python2 and python3 co-exist, you may also want to use `python3`.
which python
python -m venv ~/.venv/mlirdev
source ~/.venv/mlirdev/bin/activate

# Now the `python` command will resolve to your virtual environment and
# packages will be installed there.
python -m pip install pybind11 numpy

# Now run `cmake`, `ninja`, et al.
```

For interactive use, it is sufficient to add the `python` directory in your
`build/` directory to the `PYTHONPATH`. Typically:

```shell
export PYTHONPATH=$(cd build && pwd)/python
```

## Design

### Use cases

There are likely two primary use cases for the MLIR python bindings:

1. Support users who expect that an installed version of LLVM/MLIR will yield
   the ability to `import mlir` and use the API in a pure way out of the box.

1. Downstream integrations will likely want to include parts of the API in their
   private namespace or specially built libraries, probably mixing it with other
   python native bits.

### Composable modules

In order to support use case \#2, the Python bindings are organized into
composable modules that downstream integrators can include and re-export into
their own namespace if desired. This forces several design points:

* Separate the construction/populating of a `py::module` from `PYBIND11_MODULE`
  global constructor.

* Introduce headers for C++-only wrapper classes as other related C++ modules
  will need to interop with it.

* Separate any initialization routines that depend on optional components into
  its own module/dependency (currently, things like `registerAllDialects` fall
  into this category).

There are a lot of co-related issues of shared library linkage, distribution
concerns, etc that affect such things. Organizing the code into composable
modules (versus a monolithic `cpp` file) allows the flexibility to address many
of these as needed over time. Also, compilation time for all of the template
meta-programming in pybind scales with the number of things you define in a
translation unit. Breaking into multiple translation units can significantly aid
compile times for APIs with a large surface area.

### Submodules

Generally, the C++ codebase namespaces most things into the `mlir` namespace.
However, in order to modularize and make the Python bindings easier to
understand, sub-packages are defined that map roughly to the directory structure
of functional units in MLIR.

Examples:

* `mlir.ir`
* `mlir.passes` (`pass` is a reserved word :( )
* `mlir.dialect`
* `mlir.execution_engine` (aside from namespacing, it is important that
  "bulky"/optional parts like this are isolated)

In addition, initialization functions that imply optional dependencies should
be in underscored (notionally private) modules such as `_init` and linked
separately. This allows downstream integrators to completely customize what is
included "in the box" and covers things like dialect registration,
pass registration, etc.

### Loader

LLVM/MLIR is a non-trivial python-native project that is likely to co-exist with
other non-trivial native extensions. As such, the native extension (i.e. the
`.so`/`.pyd`/`.dylib`) is exported as a notionally private top-level symbol
(`_mlir`), while a small set of Python code is provided in `mlir/__init__.py`
and siblings which loads and re-exports it. This split provides a place to stage
code that needs to prepare the environment *before* the shared library is loaded
into the Python runtime, and also provides a place that one-time initialization
code can be invoked apart from module constructors.

To start with the `mlir/__init__.py` loader shim can be very simple and scale to
future need:

```python
from _mlir import *
```

### Use the C-API

The Python APIs should seek to layer on top of the C-API to the degree possible.
Especially for the core, dialect-independent parts, such a binding enables
packaging decisions that would be difficult or impossible if spanning a C++ ABI
boundary. In addition, factoring in this way side-steps some very difficult
issues that arise when combining RTTI-based modules (which pybind derived things
are) with non-RTTI polymorphic C++ code (the default compilation mode of LLVM).

### Ownership in the Core IR

There are several top-level types in the core IR that are strongly owned by their python-side reference:

* `PyContext` (`mlir.ir.Context`)
* `PyModule` (`mlir.ir.Module`)
* `PyOperation` (`mlir.ir.Operation`) - but with caveats

All other objects are dependent. All objects maintain a back-reference
(keep-alive) to their closest containing top-level object. Further, dependent
objects fall into two categories: a) uniqued (which live for the life-time of
the context) and b) mutable. Mutable objects need additional machinery for
keeping track of when the C++ instance that backs their Python object is no
longer valid (typically due to some specific mutation of the IR, deletion, or
bulk operation).

### Optionality and argument ordering in the Core IR

The following types support being bound to the current thread as a context manager:

* `PyLocation` (`loc: mlir.ir.Location = None`)
* `PyInsertionPoint` (`ip: mlir.ir.InsertionPoint = None`)
* `PyMlirContext` (`context: mlir.ir.Context = None`)

In order to support composability of function arguments, when these types appear
as arguments, they should always be the last and appear in the above order and
with the given names (which is generally the order in which they are expected to
need to be expressed explicitly in special cases) as necessary. Each should
carry a default value of `py::none()` and use either a manual or automatic
conversion for resolving either with the explicit value or a value from the
thread context manager (i.e. `DefaultingPyMlirContext` or
`DefaultingPyLocation`).

The rationale for this is that in Python, trailing keyword arguments to the
*right* are the most composable, enabling a variety of strategies such as kwarg
passthrough, default values, etc. Keeping function signatures composable
increases the chances that interesting DSLs and higher level APIs can be
constructed without a lot of exotic boilerplate.

Used consistently, this enables a style of IR construction that rarely needs to
use explicit contexts, locations, or insertion points but is free to do so when
extra control is needed.

#### Operation hierarchy

As mentioned above, `PyOperation` is special because it can exist in either a
top-level or dependent state. The life-cycle is unidirectional: operations can
be created detached (top-level) and once added to another operation, they are
then dependent for the remainder of their lifetime. The situation is more
complicated when considering construction scenarios where an operation is added
to a transitive parent that is still detached, necessitating further accounting
at such transition points (i.e. all such added children are initially added to
the IR with a parent of their outer-most detached operation, but then once it is
added to an attached operation, they need to be re-parented to the containing
module).

Due to the validity and parenting accounting needs, `PyOperation` is the owner
for regions and blocks and needs to be a top-level type that we can count on not
aliasing. This let's us do things like selectively invalidating instances when
mutations occur without worrying that there is some alias to the same operation
in the hierarchy. Operations are also the only entity that are allowed to be in
a detached state, and they are interned at the context level so that there is
never more than one Python `mlir.ir.Operation` object for a unique
`MlirOperation`, regardless of how it is obtained.

The C/C++ API allows for Region/Block to also be detached, but it simplifies the
ownership model a lot to eliminate that possibility in this API, allowing the
Region/Block to be completely dependent on its owning operation for accounting.
The aliasing of Python `Region`/`Block` instances to underlying
`MlirRegion`/`MlirBlock` is considered benign and these objects are not interned
in the context (unlike operations).

If we ever want to re-introduce detached regions/blocks, we could do so with new
"DetachedRegion" class or similar and also avoid the complexity of accounting.
With the way it is now, we can avoid having a global live list for regions and
blocks. We may end up needing an op-local one at some point TBD, depending on
how hard it is to guarantee how mutations interact with their Python peer
objects. We can cross that bridge easily when we get there.

Module, when used purely from the Python API, can't alias anyway, so we can use
it as a top-level ref type without a live-list for interning. If the API ever
changes such that this cannot be guaranteed (i.e. by letting you marshal a
native-defined Module in), then there would need to be a live table for it too.

## Style

In general, for the core parts of MLIR, the Python bindings should be largely
isomorphic with the underlying C++ structures. However, concessions are made
either for practicality or to give the resulting library an appropriately
"Pythonic" flavor.

### Properties vs get\*() methods

Generally favor converting trivial methods like `getContext()`, `getName()`,
`isEntryBlock()`, etc to read-only Python properties (i.e. `context`). It is
primarily a matter of calling `def_property_readonly` vs `def` in binding code,
and makes things feel much nicer to the Python side.

For example, prefer:

```c++
m.def_property_readonly("context", ...)
```

Over:

```c++
m.def("getContext", ...)
```

### __repr__ methods

Things that have nice printed representations are really great :)  If there is a
reasonable printed form, it can be a significant productivity boost to wire that
to the `__repr__` method (and verify it with a [doctest](#sample-doctest)).

### CamelCase vs snake\_case

Name functions/methods/properties in `snake_case` and classes in `CamelCase`. As
a mechanical concession to Python style, this can go a long way to making the
API feel like it fits in with its peers in the Python landscape.

If in doubt, choose names that will flow properly with other
[PEP 8 style names](https://pep8.org/#descriptive-naming-styles).

### Prefer pseudo-containers

Many core IR constructs provide methods directly on the instance to query count
and begin/end iterators. Prefer hoisting these to dedicated pseudo containers.

For example, a direct mapping of blocks within regions could be done this way:

```python
region = ...

for block in region:

  pass
```

However, this way is preferred:

```python
region = ...

for block in region.blocks:

  pass

print(len(region.blocks))
print(region.blocks[0])
print(region.blocks[-1])
```

Instead of leaking STL-derived identifiers (`front`, `back`, etc), translate
them to appropriate `__dunder__` methods and iterator wrappers in the bindings.

Note that this can be taken too far, so use good judgment. For example, block
arguments may appear container-like but have defined methods for lookup and
mutation that would be hard to model properly without making semantics
complicated. If running into these, just mirror the C/C++ API.

### Provide one stop helpers for common things

One stop helpers that aggregate over multiple low level entities can be
incredibly helpful and are encouraged within reason. For example, making
`Context` have a `parse_asm` or equivalent that avoids needing to explicitly
construct a SourceMgr can be quite nice. One stop helpers do not have to be
mutually exclusive with a more complete mapping of the backing constructs.

## Testing

Tests should be added in the `test/Bindings/Python` directory and should
typically be `.py` files that have a lit run line.

We use `lit` and `FileCheck` based tests:

* For generative tests (those that produce IR), define a Python module that
  constructs/prints the IR and pipe it through `FileCheck`.
* Parsing should be kept self-contained within the module under test by use of
  raw constants and an appropriate `parse_asm` call.
* Any file I/O code should be staged through a tempfile vs relying on file
  artifacts/paths outside of the test module.
* For convenience, we also test non-generative API interactions with the same
  mechanisms, printing and `CHECK`ing as needed.

### Sample FileCheck test

```python
# RUN: %PYTHON %s | mlir-opt -split-input-file | FileCheck

# TODO: Move to a test utility class once any of this actually exists.
def print_module(f):
  m = f()
  print("// -----")
  print("// TEST_FUNCTION:", f.__name__)
  print(m.to_asm())
  return f

# CHECK-LABEL: TEST_FUNCTION: create_my_op
@print_module
def create_my_op():
  m = mlir.ir.Module()
  builder = m.new_op_builder()
  # CHECK: mydialect.my_operation ...
  builder.my_op()
  return m
```

## Integration with ODS

The MLIR Python bindings integrate with the tablegen-based ODS system for
providing user-friendly wrappers around MLIR dialects and operations. There
are multiple parts to this integration, outlined below. Most details have
been elided: refer to the build rules and python sources under `mlir.dialects`
for the canonical way to use this facility.

### Generating `{DIALECT_NAMESPACE}.py` wrapper modules

Each dialect with a mapping to python requires that an appropriate
`{DIALECT_NAMESPACE}.py` wrapper module is created. This is done by invoking
`mlir-tablegen` on a python-bindings specific tablegen wrapper that includes
the boilerplate and actual dialect specific `td` file. An example, for the
`StandardOps` (which is assigned the namespace `std` as a special case):

```tablegen
#ifndef PYTHON_BINDINGS_STANDARD_OPS
#define PYTHON_BINDINGS_STANDARD_OPS

include "mlir/Bindings/Python/Attributes.td"
include "mlir/Dialect/StandardOps/IR/Ops.td"

#endif
```

In the main repository, building the wrapper is done via the CMake function
`add_mlir_dialect_python_bindings`, which invokes:

```
mlir-tablegen -gen-python-op-bindings -bind-dialect={DIALECT_NAMESPACE} \
    {PYTHON_BINDING_TD_FILE}
```

### Extending the search path for wrapper modules

When the python bindings need to locate a wrapper module, they consult the
`dialect_search_path` and use it to find an appropriately named module. For
the main repository, this search path is hard-coded to include the
`mlir.dialects` module, which is where wrappers are emitted by the abobe build
rule. Out of tree dialects and add their modules to the search path by calling:

```python
mlir._cext.append_dialect_search_prefix("myproject.mlir.dialects")
```

### Wrapper module code organization

The wrapper module tablegen emitter outputs:

* A `_Dialect` class (extending `mlir.ir.Dialect`) with a `DIALECT_NAMESPACE`
  attribute.
* An `{OpName}` class for each operation (extending `mlir.ir.OpView`).
* Decorators for each of the above to register with the system.

Note: In order to avoid naming conflicts, all internal names used by the wrapper
module are prefixed by `_ods_`.

Each concrete `OpView` subclass further defines several attributes:

* `OPERATION_NAME` attribute with the `str` fully qualified operation name
  (i.e. `std.absf`).
* An `__init__` method for the *default builder* if one is defined or inferred
  for the operation.
* `@property` getter for each operand or result (using an auto-generated name
  for unnamed of each).
* `@property` getter, setter and deleter for each declared attribute.

#### Builders

Presently, only a single, default builder is mapped to the `__init__` method.
Generalizing this facility is under active development. It currently accepts
arguments:

* One argument for each declared result:
  * For single-valued results: Each will accept an `mlir.ir.Type`.
  * For variadic results: Each will accept a `List[mlir.ir.Type]`.
* One argument for each declared operand or attribute:
  * For single-valued operands: Each will accept an `mlir.ir.Value`.
  * For variadic operands: Each will accept a `List[mlir.ir.Value]`.
  * For attributes, it will accept an `mlir.ir.Attribute`.
* Trailing usage-specific, optional keyword arguments:
  * `loc`: An explicit `mlir.ir.Location` to use. Defaults to the location
    bound to the thread (i.e. `with Location.unknown():`) or an error if none
    is bound nor specified.
  * `context`: An explicit `mlir.ir.Context` to use. Default to the context
    bound to the thread (i.e. `with Context():` or implicitly via `Location` or
    `InsertionPoint` context managers) or an error if none is bound nor
    specified.
