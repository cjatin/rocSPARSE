# rocSPARSE

rocSPARSE exposes a common interface that provides Basic Linear Algebra Subroutines for sparse computation implemented on top of AMD's Radeon Open eCosystem Platform [ROCm][] runtime and toolchains. rocSPARSE is created using the [HIP][] programming language and optimized for AMD's latest discrete GPUs.

## Documentation

The latest rocSPARSE documentation and API description can be found [here][] or downloaded as [pdf][].

Run the commands below to build documentation locally:

```
cd docs
pip3 install -r sphinx/requirements.txt
python3 -m sphinx -T -E -b html -d _build/doctrees -D language=en . _build/html
```

## Requirements

* Git
* CMake (3.5 or later)
* AMD [ROCm] 3.5 platform or later

Optional:
* [GTest][]
  * Required for tests.
  * Use GTEST_ROOT to specify GTest location.
  * If [GTest][] is not found, it will be downloaded and built automatically.

## Quickstart rocSPARSE build and install

#### Install script

You can build rocSPARSE using the *install.sh* script
```
# Clone rocSPARSE using git
git clone https://github.com/ROCmSoftwarePlatform/rocSPARSE.git

# Go to rocSPARSE directory
cd rocSPARSE

# Run install.sh script
# Command line options:
#   -h|--help         - prints help message
#   -i|--install      - install after build
#   -d|--dependencies - install build dependencies
#   -c|--clients      - build library clients too (combines with -i & -d)
#   -g|--debug        - build with debug flag
./install.sh -dci
```

#### CMake

All compiler specifications are determined automatically. The compilation process can be performed by
```
# Clone rocSPARSE using git
git clone https://github.com/ROCmSoftwarePlatform/rocSPARSE.git

# Go to rocSPARSE directory, create and go to the build directory
cd rocSPARSE; mkdir -p build/release; cd build/release

# Configure rocSPARSE
# Build options:
#   BUILD_CLIENTS_TESTS      - build tests (OFF)
#   BUILD_CLIENTS_BENCHMARKS - build benchmarks (OFF)
#   BUILD_CLIENTS_SAMPLES    - build examples (ON)
#   BUILD_VERBOSE            - verbose output (OFF)
#   BUILD_SHARED_LIBS        - build rocSPARSE as a shared library (ON)
CXX=/opt/rocm/bin/hipcc cmake -DBUILD_CLIENTS_TESTS=ON ../..

# Build
make

# Install
[sudo] make install
```

## Unit tests

To run unit tests, rocSPARSE has to be built with option -DBUILD_CLIENTS_TESTS=ON.
```
# Go to rocSPARSE build directory
cd rocSPARSE; cd build/release

# Run all tests
./clients/staging/rocsparse-test
```

## Benchmarks

To run benchmarks, rocSPARSE has to be built with option -DBUILD_CLIENTS_BENCHMARKS=ON.
```
# Go to rocSPARSE build directory
cd rocSPARSE/build/release

# Run benchmark, e.g.
./clients/staging/rocsparse-bench -f hybmv --laplacian-dim 2000 -i 200
```

## Support

Please use [the issue tracker][] for bugs and feature requests.

## License

The [license file][] can be found in the main repository.

[ROCm]: https://github.com/RadeonOpenCompute/ROCm
[HIP]: https://github.com/GPUOpen-ProfessionalCompute-Tools/HIP/
[GTest]: https://github.com/google/googletest
[the issue tracker]: https://github.com/ROCmSoftwarePlatform/rocSPARSE/issues
[license file]: https://github.com/ROCmSoftwarePlatform/rocSPARSE
[here]: https://rocsparse.readthedocs.io
[pdf]: https://rocsparse.readthedocs.io/_/downloads/en/master/pdf/
