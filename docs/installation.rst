Installation
============

The library has no external dependencies and can be added as submodule to your
project. If you are using CMake just add these lines to your CMakeLists.txt

.. code-block:: cmake

    option(BUILD_TESTING "" OFF)
    add_subdirectory(${PATH_TO_LIBRARIES}/uwan)
    target_link_libraries(${PROJECT_NAME} uwan)


Building the library only:

.. code-block:: bash

    cmake -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build

Run tests:

.. code-block:: bash

    cmake --build build --target test
