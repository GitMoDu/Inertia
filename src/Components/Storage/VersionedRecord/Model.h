#ifndef _INERTIA_COMPONENTS_STORAGE_VERSIONED_RECORD_MODEL_h_
#define _INERTIA_COMPONENTS_STORAGE_VERSIONED_RECORD_MODEL_h_

#include <stddef.h>
#include <stdint.h>

namespace Inertia
{
	namespace Components
	{
		namespace Storage
		{
			namespace VersionedRecord
			{
				enum class CodecResultEnum : uint8_t
				{
					Success,
					InvalidArgument,
					BufferTooSmall,
					InvalidSize,
					UnsupportedVersion,
					ChecksumMismatch,
					MigrationFailed
				};

				struct HeaderStruct
				{
					uint32_t Checksum;
					uint16_t Version;
					uint16_t Size;
				};

				/// <summary>Fixed-capacity byte envelope for storing encoded versioned records in typed backends.</summary>
				/// <typeparam name="Capacity">Maximum encoded record size reserved across firmware versions.</typeparam>
				template<size_t Capacity>
				struct Envelope
				{
					static_assert(Capacity > 0 && Capacity <= UINT16_MAX,
						"Versioned-record envelope capacity must fit its uint16_t size field.");

					uint16_t Size;
					uint8_t Bytes[Capacity];
				};

				static_assert(offsetof(HeaderStruct, Checksum) == 0
					&& offsetof(HeaderStruct, Version) == sizeof(uint32_t)
					&& offsetof(HeaderStruct, Size) == sizeof(uint32_t) + sizeof(uint16_t),
					"Unexpected padding in the versioned-record header.");
				static_assert(sizeof(HeaderStruct) == sizeof(uint32_t) + 2 * sizeof(uint16_t)
					&& alignof(HeaderStruct) == alignof(uint32_t),
					"Unexpected versioned-record header size or alignment.");
			}
		}
	}
}

#endif