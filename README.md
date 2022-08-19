# BinaryIO : Library for managing binary data streams in C++

BinaryIO is a small base C++ library for a unified binary stream interface with
some helpers for easily implementing custom binary file formats.

[assetio/binary_assert.hpp](include/assetio/binary_assert.hpp): Contains the `binaryIOAssert` assertion macro.

[assetio/binary_chunk.hpp](include/assetio/binary_chunk.hpp): Contains datatypes for a simple chunk based binary file format.

[assetio/binary_stream.hpp](include/assetio/binary_stream.hpp): Contains the base interfaces for writing and reading binary data with some utilities for read/write-ing integers with a little/big endianness.

- `IOResult`       : Enum containing all error codes possible from this library.
- `IByteWriter`    : Interface for writing to a binary stream.
- `ByteWriterView` : Non-owning IByteWriter adaptor.
- `IByteReader`    : Interface for reading a binary stream.
- `swapEndian`     : Function for swapping endianness of an integer.
- `writeLE`        : Function for writing an integer in little endian format.
- `writeBE`        : Function for writing an integer in big endian format.
- `readLE`         : Function for reading an integer in little endian format.
- `readBE`         : Function for reading an integer in big endian format.

[assetio/binary_stream_ext.hpp](include/assetio/binary_stream_ext.hpp) : Contains extensions not needed in the core api for a smaller base header.

- `byteWriterViewFromVector` : Function for creating a buffer view from a standard vector.
- `CFileBufferedByteReader`  : C File IByteReader implementation.

[assetio/rel_ptr.hpp](include/assetio/rel_ptr.hpp): Contains a pointer type that stores the relative offset from itself to the pointer object for making memory mappable binary file formats.

- `rel_ptr<IntType, T>`                 : Class for the relative pointer.
- `rel_array<CountIntType, RelPtrType>` : Class for an array that contains data relative to it's own address.
