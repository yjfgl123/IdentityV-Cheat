#pragma once

#include <vector>
#include "ImGui/imgui.h"
#include "Utils/Utils.h"
#include <map>
#include <functional>
#include "Memory/mem.h"
#include <optional>
#include <array>
#include "types.h"

namespace IDVGame {
	enum ObjectType {
		INVALID,
		CIVILIAN,
		BUTCHER,
		HOOK,
		PANEL
	};

	struct ObjectDef {
		std::string name;
		ObjectType type;
		std::string identifier;
		ObjectDef(std::string n, ObjectType t, std::string id)
			: name(std::move(n)), type(t), identifier(std::move(id)) {
		}
	};
	struct GameObject;
	extern std::vector<ObjectDef> defs; 
	extern ObjectDef invalid_def;
	extern std::map<ObjectType, std::function<bool(GameObject const*)>> extra_def_check;

	struct Matrix {
		float matrix[16];
	};
	struct Position : public Vector3{
	};
	struct Direction : public Vector3{
	};
	struct ColBox {
		Position start;
		Position end;
	};
	struct ModelObject : Memory::MemoryPointer{//neox::world::SceneRoot
		inline std::optional<float> getHeight() {
			return read<float>(0x214);
		}
		inline std::optional<ColBox> getColbox() {
			return read<ColBox>(0x23C);
		}
		inline std::optional<Direction> getDirection() {
			return read<Direction>(0xF8);
		}
		inline std::optional<Position> getPosition() {
			return read<Position>(0xE0);
		}
	};

	struct GameObject : Memory::MemoryPointer{//neox::world::ModelSkeletal

		std::optional<std::array<Memory::MemoryPointer, 2>> getHashChunk() const {
			auto a1 = read_pointer(0x9A0);
			if (a1.is_null()) {
				return std::nullopt;
			}
			auto start = a1.read_pointer(0x2D0);
			auto end = a1.read_pointer(0x2D8);
			if (start.is_null() || end.is_null()) {
				return std::nullopt;
			}
			return std::array{ start,end };
		}

		inline std::optional<uint64_t> getHash() const{
			auto chunk = getHashChunk();
			if (!chunk) {
				return std::nullopt;
			}

			return Memory::hash_memory_region((*chunk)[0], (*chunk)[1]);
		}

		inline std::optional<uint64_t> getHashedLength() const{
			auto chunk = getHashChunk();
			if (!chunk) {
				return std::nullopt;
			}
			return ((*chunk)[1].get() - (*chunk)[0].get()) >> 5;
		}

		inline std::optional<ModelObject> getModelObject() const{
			auto p = read_pointer(0x40);
			if (p.is_null() == false) {
				return ModelObject(p);
			}
			else {
				return std::nullopt;
			}
		}

		inline std::optional<std::string> getIdentifier() const {
			uint32_t off[] = {0x170, 0x0, 0x58, 0xF8, 0x28,0x8,0x0};
			Memory::MemoryPointer ptr = Memory::get_pointer(*this, off, sizeof(off) / sizeof(uint32_t));
			if (ptr.is_null() == false) {
				return Memory::read_str(ptr);
			}
			else {
				return std::nullopt;
			}
		}

		inline ObjectDef const* getDef(bool no_zero = true) const {
			if (no_zero) {
				auto model = getModelObject();
				if (model) {
					auto pos = model->getPosition();
					if (pos && (*pos).x == 0.0 && (*pos).y == 0.0 && (*pos).z == 0.0) {
						return &invalid_def;
					}
				}
			}
			auto identifier = getIdentifier();
			if (!identifier) {
				return &invalid_def;
			}
			else {
				for (ObjectDef const& i : defs) {
					if (strstr((*identifier).c_str(), i.identifier.c_str())) {
						if (extra_def_check.contains(i.type)) {
							if (extra_def_check[i.type](this) == false) {
								return &invalid_def;
							}
						}
						return &i;
					}
				}
				return &invalid_def;
			}
		}
	};




	std::vector<GameObject> readObjectArray(Memory::MemoryPointer base);
	std::optional<Matrix> readMatrix(Memory::MemoryPointer base);
	std::optional<Position> readLocalPosition(Memory::MemoryPointer base);
	bool worldToScreen(Matrix const& viewMatrix, float screenWidth, float screenHeight, Position const& worldPos, float* outScreenPos);
}