#pragma once

#include <memory>
#include <variant>
#include "util.hpp"

namespace asmio {

	template <typename T, typename A>
	concept codec_for = requires (class ChunkBuffer& buffer, const A& arg) { T::encode(buffer, arg); };

	class ChunkBuffer {

		public:

			const char* name = "";
			using Ptr = std::shared_ptr<ChunkBuffer>;

		protected:

			using Linker = std::function<void(uint8_t* target)>;

			struct Link {
				ChunkBuffer* target;
				uint32_t offset; // offset into shared_bytes
				Linker linker;
			};

			enum CacheState {
				MISSING,
				CACHING,
				PRESENT,
			};

			// cache
			mutable CacheState state = MISSING;
			mutable int64_t m_size = -1;
			mutable int64_t m_offset = -1;

			uint32_t alignment = 1;
			std::endian endianness = std::endian::little;

			ChunkBuffer* m_parent = nullptr;
			ChunkBuffer* m_root = this;

			struct Appender {
				std::vector<uint8_t>& buffer;
				uint32_t* size_ptr;

				/// Append buffer of given length
				void write(uint8_t* data, size_t size) const;

				/// Append a single byte the given number of times
				void write(uint8_t value, size_t size) const;

				Appender(std::vector<uint8_t>& buffer, uint32_t* size_ptr);
			};

			enum region_type : uint8_t {
				UNSET = 1,
				ARRAY = 2,
				CHUNK = 4,
			};

			struct Array {
				uint32_t offset;
				uint32_t size;

				constexpr Array() = default;
				constexpr Array(size_t offset, size_t size)
					: offset(offset), size(size) {
				}
			};

		private:

			std::vector<uint8_t> shared_bytes;
			std::vector<std::variant<Array, Ptr>> m_regions;
			std::vector<Link> m_linkers; // only non-empty for root chunks
			region_type last_region = UNSET;

		protected:

			/// Switches the buffer to raw bytes mode
			Appender begin_bytes();

			/// Creates a new sub-chunk
			Ptr begin_chunk(uint32_t align, std::endian endian, const char* name);

			/// Enable cache, any changes to the buffer lengths at this point will break the tree
			void freeze();

			/// Write this chunk to the buffer
			void bake(std::vector<uint8_t>& output) const;

			/// Add a new linker targeting the next byte to be inserted into this chunk
			void add_link(const Linker& linker);

			/// Set the root chunk of this tree recursively
			void set_root(ChunkBuffer* chunk);

		public:

			ChunkBuffer() = default;
			ChunkBuffer(uint32_t align, std::endian endian, ChunkBuffer* root, ChunkBuffer* parent) noexcept;
			ChunkBuffer(std::endian endian, uint32_t align = 1) noexcept;
			ChunkBuffer(uint32_t align, std::endian endian = std::endian::little) noexcept;

			/**
			 * Get the index of a child chunk
			 * or throw if there is no such child.
			 */
			int index(const ChunkBuffer* child) const;

			/**
			 * Get the index of this chunk in it's parent,
			 * or zero for root chunk.
			 */
			int index() const;

			/**
			 * Get a number of buffers and chunks
			 * that are the children of this chunk.
			 */
			size_t regions() const;

			/**
			 * The size in bytes of this chunk's content, ignoring required alignment
			 * and any data in sub-chunks.
			 */
			size_t bytes() const;

			/**
			 * Total size of this chunk, excluding padding resulting from alignment
			 * but including the padding og any data in sub-chunks (and their alignment paddings).
			 */
			size_t size(size_t offset) const;

			/**
			 * Same as size() but including padding required by alignment,
			 * this is the real size required by this buffer when saved.
			 */
			size_t outer(size_t offset) const;

			/**
			 * Total size of this chunk, padding resulting from alignment
			 * and any data in sub-chunks (and their alignment paddings).
			 */
			size_t size() const;

			/**
			 * Compute the offset of this chunk, the
			 * resulting offsets point at the first byte of the content.
			 */
			size_t offset() const;

			/**
			 * Compute the offset of a particular child buffer from the start of the
			 * chunk tree. The offsets point at the first byte of the chunk content.
			 */
			size_t offset(const ChunkBuffer* child) const;

			/**
			 * Get the root chunk of this tree,
			 * this can safely be used on root chunk as well.
			 */
			ChunkBuffer* root();

		public:

			/**
			 * Align chunk contents to a fixed number of bytes using
			 * the null byte. This doesn't take absolute alignment into account.
			 */
			void align(int bytes) {
				int padding = bytes - (int) shared_bytes.size();

				if (padding > 0) {
					push(padding, 0);
				}
			}

			/**
			 * Write a string to the chunk,
			 * including the C-String null byte at the end.
			 */
			ChunkBuffer& write(const std::string& name) {
				return write((void*) name.c_str(), name.size() + 1);
			}

			/**
			 * Write a vector of values,
			 * this will respect the endianness for integer values.
			 */
			template <typename T>
			ChunkBuffer& write(const std::vector<T>& values) {
				if constexpr (sizeof(T) == 1) {
					return write((void*) values.data(), values.size());
				}

				for (const auto& value : values) {
					put<T>(value);
				}

				return *this;
			}

			/**
			 * Write a buffer of set number of bytes into the chunk,
			 * using this method circumvents endianness settings.
			 */
			ChunkBuffer& write(void* bytes, size_t size) {
				begin_bytes().write(static_cast<uint8_t*>(bytes), size);
				return *this;
			}

			/**
			 * Write a specific byte to the buffer a set number of times,
			 * by default a null byte is used.
			 */
			ChunkBuffer& push(size_t size, uint8_t value = 0) {
				begin_bytes().write(value, size);
				return *this;
			}

			/**
			 * This method doesn't make the type explicit during invocation,
			 * consider using the more explicit put<type>() method instead.
			 */
			template <typename... T>
			ChunkBuffer& insert(T... value) {
				(write(reinterpret_cast<void*>(&value), sizeof(T)), ...);
				return *this;
			}

			/**
			 * Put can use a codec to specify how the given value
			 * should be stored, this is the method used when a codec is used.
			 */
			template <typename T, typename... A>
			requires (codec_for<T, A> && ...)
			ChunkBuffer& put(const A&... value) {
				(T::encode(*this, value), ...);
				return *this;
			}

			/**
			 * Safer variant of insert(), use this method
			 * when possible to avoid concealing the type.
			 */
			template <trivially_copyable T, trivially_copyable... A>
			requires ((!codec_for<T, A> && castable<T, A>) && ...)
			ChunkBuffer& put(const A&... value) {
				if constexpr (std::is_integral_v<T> && sizeof(T) > 1) {
					return insert(util::native_to_endian(static_cast<T>(value), endianness)...);
				}

				return insert(static_cast<T>(value)...);
			}

			/**
			 * Creates a link, the provided getter function will be invoked just before converting the buffer to
			 * a contiguous byte stream. This function respects endianness.
			 */
			template <typename T>
			ChunkBuffer link(std::function<T()> getter) {
				add_link([getter, this] (void* target) {
					*static_cast<T*>(target) = util::native_to_endian(getter(), endianness);
				});

				return push(sizeof(T));
			}

			/**
			 * Creates a link, the provided linker function will be invoked just before converting the buffer to
			 * a contiguous byte stream. This function DOES NOT respects endianness, unless that is manually implemented by the user.
			 */
			template <typename T>
			ChunkBuffer link(std::function<void(T&)> linker) {
				add_link([linker] (void* target) {
					linker(*static_cast<T*>(target));
				});

				return push(sizeof(T));
			}

			/**
			 * Adopt a orphan chunk into the tree and assign it this node as a parent,
			 * if the chunk already has a parent it will be rejected.
			 */
			ChunkBuffer& adopt(const Ptr& orphan);

			/**
			 * Create a new chunk as the child of this one,
			 * the endianness will be inherited.
			 */
			Ptr chunk(const char* name);

			/**
			 * Create a new chunk as the child of this one,
			 * the endianness will be inherited.
			 */
			Ptr chunk(uint32_t align = 1, const char* name = nullptr);

			/**
			 * Create a new chunk as the child of this one,
			 * the inherited endianness will be overridden.
			 */
			Ptr chunk(std::endian endian, uint32_t align = 1, const char* name = nullptr);

			/**
			 * Bake this chunk tree into a contiguous byte buffer,
			 * all links will be resolved at this point, this mutates the internal structure.
			 */
			std::vector<uint8_t> bake();

	};

}