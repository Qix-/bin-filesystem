# Filesystem.hpp

> Originally from [wjakob/filesystem](https://github.com/wjakob/filesystem)

This is a tiny, self-contained Filesystem library for manipulating paths and the
system's filesystem.

See `filesystem.hpp` (which contains all relevant code) for API and `path_demo.cpp`
for usage examples.

## Usage

To use in your Meson project, check this repository out to `subprojects/filesystem` and add
the following lines to your `meson.build`:

```meson
filesystem = subproject('filesystem').get_variable('dep')
executable('my-app', 'app.cc', dependencies: [filesystem])
```

# License
Licensed under the [BSD 2-Clause License](LICENSE).
