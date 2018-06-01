# physfs_cxx [![Codacy Badge](https://api.codacy.com/project/badge/Grade/070ac7666e0c40e9aede0859db118bd5)](https://www.codacy.com/app/zie87/physfs_cxx?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=zie87/physfs_cxx&amp;utm_campaign=Badge_Grade)

[![Build status](https://ci.appveyor.com/api/projects/status/b5qv3b68oc5dnfas?svg=true)](https://ci.appveyor.com/project/zie87/physfs-cxx)
[![Build Status](https://travis-ci.org/zie87/physfs_cxx.svg?branch=master)](https://travis-ci.org/zie87/physfs_cxx)
[![Codecov Status](https://codecov.io/gh/zie87/physfs_cxx/branch/master/graph/badge.svg)](https://codecov.io/gh/zie87/physfs_cxx)
[![Coverall Status](https://coveralls.io/repos/github/zie87/physfs_cxx/badge.svg)](https://coveralls.io/github/zie87/physfs_cxx)

Header only modern C++ wrapper for [physfs](https://icculus.org/physfs/)


## libraries used
- [PhysicsFS](https://icculus.org/physfs/) as base for this project.
- [catch](https://github.com/philsquared/Catch) as the test framework.  (as
submodule)


## Generate project

```shell
  cmake -H. -BBuild
```

Auto detect everything.

If you like to set a implicit compiler set the variable CXX=${COMPILER}, for
example COMPILER could be gcc, clang and so on.

Auto detect in Windows usually generate a Visual Studio project since msbuild
require it, but in OSX does not generate and XCode project, since is not
required for compiling using XCode clang.

Specify build type debug/release

```shell
  # generate a debug project
  cmake -H. -BBuild -DCMAKE_BUILD_TYPE=Debug
  # generate a release project
  cmake -H. -BBuild -DCMAKE_BUILD_TYPE=Release
```

## Build

From the Build folder

```shell
  # build the default build type (in multi build types usually debug)
  cmake --build .
  # build a specific build type
  cmake --build . --config Release
```
## Run tests

From the Build folder

```shell
  # run all test using the default build type
  ctest -V
  # run all test in Release build type
  ctest -V -C Release
```
