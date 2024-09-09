# BinaryIO : C++ library for managing binary data streams

![Windows Build Badge](https://github.com/BluFedora/BinaryIO/actions/workflows/build_windows.yml/badge.svg)
![Linux Build Badge](https://github.com/BluFedora/BinaryIO/actions/workflows/build_linux.yml/badge.svg)
![macOS Build Badge](https://github.com/BluFedora/BinaryIO/actions/workflows/build_macos.yml/badge.svg)

BinaryIO is a small C++ library for a unified binary stream interface
with some helpers for easily implementing custom binary file formats.

[binaryio/binary_assert.hpp](include/binaryio/binary_assert.hpp): Contains the `binaryIOAssert` assertion macro.

[binaryio/binary_chunk.hpp](include/binaryio/binary_chunk.hpp): Contains datatypes for a simple chunk based binary file format.

[binaryio/binary_stream.hpp](include/binaryio/binary_stream.hpp): Contains the base interfaces for writing and reading binary data with some utilities for read/write-ing integers with a little/big endianness.

- `BufferedIO` : Interface for a no copy read operation for certain `IOStream`s.
- `IOStream`   : Interface for reading and writing to a binary stream.
- `writeLE`    : Function for writing an integer in little endian format.
- `writeBE`    : Function for writing an integer in big endian format.
- `readLE`     : Function for reading an integer in little endian format.
- `readBE`     : Function for reading an integer in big endian format.

[binaryio/binary_stream_ext.hpp](include/binaryio/binary_stream_ext.hpp) : Contains extensions not needed in the core api for a smaller base header.

- `byteWriterViewFromVector` : Function for creating a buffer view from a standard vector.
- `CFileBufferedByteReader`  : C File IByteReader implementation.

[binaryio/binary_types.hpp](include/binaryio/binary_types.hpp) : Forward declarations of the types defined by this library.

- `IOSize`      : Size type used for IO operations.
- `IOOffset`    : Type used for seek operations.
- `VersionType` : Type used for the version field in the chunk binary format.
- `ChunkTypeID` : Type used for the type id field in the chunk binary format.
- `IOErrorCode` : Enum containing all error codes possible from this library.
- `SeekOrigin`  : Defined the starting point of the seek operation.
- `IOResult`    : Compressed pair of IOSize and IOErrorCode, result from most IO operations.

[binaryio/rel_ptr.hpp](include/binaryio/rel_ptr.hpp): Contains a pointer type that stores the relative offset from itself to the pointed object for making memory mappable binary file formats.

- `rel_ptr<IntType, T>`                 : Class for the relative pointer.
- `rel_array<CountIntType, RelPtrType>` : Class for an array that contains data relative to it's own address.
