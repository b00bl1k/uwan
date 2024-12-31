# Uwan Micro LoRaWAN Stack

Uwan is a simple and lightweight library for developing applications based on
LoRaWAN. It provides a basic stack for integrating LoRaWAN into your IoT
projects.

## Features
- LoRaWAN specification: 1.0.2, 1.0.3
- Supported regions: EU868, RU864
- Activation: OTAA, ABP
- Class: A
- Hardware: sx127x, sx126x

## Build
To build the library, execute the following commands:

```bash
git clone https://github.com/b00bl1k/uwan.git
cd uwan
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

To run tests:

```bash
cmake --build build --target test
```

Requirements:
- C/C++ compiler
- CMake 3.5 or higher

## License
This project is licensed under the [MIT License](LICENSE).

## Contact
If you have any questions or suggestions, please contact us:
- **Email**: alex@b00bl1k.ru
- **GitHub Issues**: [Create a new ticket](https://github.com/b00bl1k/uwan/issues/new)
- **Community Forum**: [Discussions](https://github.com/b00bl1k/uwan/discussions)
