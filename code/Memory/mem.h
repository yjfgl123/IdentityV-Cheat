#pragma once

#include <stdint.h>
#include <string>
#include <Windows.h>
#include <optional>

namespace Memory {
	bool read_memory(void* dst,uint64_t size,uint64_t address);
	bool write_memory(uint64_t address,const void* buffer, uint64_t size);

	class MemoryPointer {
	private:
		uint64_t address;
	public:
		inline MemoryPointer() {
			this->address = 0;
		}
		inline MemoryPointer(uint64_t address) {
			this->address = address;
		}
		inline MemoryPointer off(uint64_t offset) const {
			return MemoryPointer(this->address + offset);
		}
		template<typename T>
		bool write(T const& t) const {
			return write_memory((uint64_t)address, &t, sizeof(t));
		}
		template<typename T>
		std::optional<T> read() const {
			T t = T();
			if (read_memory((void*)&t, sizeof(T), address)) {
				return t;
			}
			else {
				return std::nullopt;
			}
		}
		MemoryPointer read_pointer() const {
			auto p = read<uint64_t>();
			if (p) {
				return MemoryPointer(*p);
			}
			else {
				return MemoryPointer(0);
			}
		}
		template<typename T>
		bool write(uint64_t offset, T const& t) {
			return off(offset).write<T>(t);
		}
		template<typename T>
		std::optional<T> read(uint64_t offset) const {
			return off(offset).read<T>();
		}
		MemoryPointer read_pointer(uint64_t offset) const{
			auto p = read<uint64_t>(offset);
			if (p) {
				return MemoryPointer(*p);
			}
			else {
				return MemoryPointer(0);
			}
		}
		inline uint64_t get() const {
			return address;
		}

		inline bool is_null() const {
			return address == 0;
		}

	};


	inline std::optional<uint64_t> hash_memory_region(MemoryPointer start, MemoryPointer end) {
		const uint64_t FNV_OFFSET_BASIS = 14695981039346656037ULL;
		const uint64_t FNV_PRIME = 1099511628211ULL;
		uint64_t size = end.get() - start.get();
		uint64_t hash = FNV_OFFSET_BASIS;
		for (uint64_t i = 0; i < size; i++) {
			auto d = start.read<uint8_t>(i);
			if (d) {
				hash ^= *d;
				hash *= FNV_PRIME;
			}
			else {
				return std::nullopt;
			}
		}
		return hash;
	}

	inline MemoryPointer get_pointer(MemoryPointer base, uint32_t* offsets, uint32_t level) {
		MemoryPointer current = base;
		for (uint32_t i = 0; i < level - 1; ++i) {
			if (current.get() < 0x10) {
				return MemoryPointer(0);
			}
			current = current.read_pointer(offsets[i]);
			if (current.is_null() == true) {
				return MemoryPointer(0);
			}
		}
		return current.off(offsets[level - 1]);
	}

	inline std::optional<std::string> read_str(MemoryPointer start, uint64_t max_length = 256) {
		std::string result;
		result.reserve(max_length);
		for (uint64_t i = 0; i < max_length; ++i) {
			auto c = start.read<char>(i);
			if (c) {
				if (*c == '\0') break;
				result.push_back(*c);
			}
			else {
				return std::nullopt;
			}
		}
		return result;
	}

	bool attach_process();
	void detach_process(); 
	bool init(LPCWSTR process);
	MemoryPointer get_module_base(LPCWSTR dll_name);
}