#ifndef _INERTIA_COMPONENTS_STORAGE_ALLOCATION_ALLOCATOR_h
#define _INERTIA_COMPONENTS_STORAGE_ALLOCATION_ALLOCATOR_h

#include "Model.h"

namespace Inertia
{
	namespace Components
	{
		namespace Storage
		{
			namespace Allocation
			{
				class StoreStoreParameter
				{
				public:
					template<typename... Stores>
					static constexpr size_t Count()
					{
						return sizeof...(Stores);
					}

					template<typename... Stores>
					static constexpr size_t TotalSize()
					{
						return TotalSize<0, Stores...>();
					}

					template<typename... Stores>
					static constexpr size_t AddressOf(const size_t index)
					{
						return AddressOf<0, Stores...>(index);
					}

					template<typename... Stores>
					static constexpr size_t SizeOf(const size_t index)
					{
						return SizeOf<0, Stores...>(index);
					}

				private:
					template<const size_t Depth>
					static constexpr size_t TotalSize() { return 0; }

					template<const size_t Depth, typename First, typename... Rest>
					static constexpr size_t TotalSize()
					{
						return First::GetStoreSize() + TotalSize<Depth + 1, Rest...>();
					}

					template<const size_t Depth>
					static constexpr size_t AddressOf(const size_t index) { return 0; }

					template<const size_t Depth, typename First, typename... Rest>
					static constexpr size_t AddressOf(const size_t index)
					{
						return (First::GetStoreSize() * (index > Depth)) + AddressOf<Depth + 1, Rest...>(index);
					}

					template<const size_t Depth>
					static constexpr size_t SizeOf(const size_t index) { return 0; }

					template<const size_t Depth, typename First, typename... Rest>
					static constexpr size_t SizeOf(const size_t index)
					{
						return (First::GetStoreSize() * (index == Depth)) + SizeOf<Depth + 1, Rest...>(index);
					}
				};

				template<typename A, typename B>
				struct IsSameType { static constexpr bool Value = false; };

				template<typename A>
				struct IsSameType<A, A> { static constexpr bool Value = true; };

				template<uint16_t BaseAddress, typename... Stores>
				struct StoreAllocator
				{
					static constexpr size_t StoreCount = StoreStoreParameter::Count<Stores...>();
					static constexpr size_t TotalUsed = StoreStoreParameter::TotalSize<Stores...>();

					static constexpr size_t GetCount() { return StoreCount; }
					static constexpr size_t GetTotalUsed() { return TotalUsed; }

					static constexpr uint16_t GetAddress(const size_t index)
					{
						return static_cast<uint16_t>(BaseAddress + StoreStoreParameter::AddressOf<Stores...>(index));
					}

					static constexpr size_t GetSize(const size_t index)
					{
						return StoreStoreParameter::SizeOf<Stores...>(index);
					}

					template<typename Target>
					static constexpr uint16_t AddressOf()
					{
						return static_cast<uint16_t>(BaseAddress + AddressOfType<Target, Stores...>());
					}

				private:
					template<typename Target>
					static constexpr size_t AddressOfType() { return 0; }

					template<typename Target, typename First, typename... Rest>
					static constexpr size_t AddressOfType()
					{
						return IsSameType<Target, First>::Value
							? 0
							: First::GetStoreSize() + AddressOfType<Target, Rest...>();
					}
				};
			}
		}
	}
}
#endif