#ifndef _INERTIA_COMPONENTS_STORAGE_VERSIONED_RECORD_CODEC_h_
#define _INERTIA_COMPONENTS_STORAGE_VERSIONED_RECORD_CODEC_h_

#include "Model.h"
#include <Fletcher32.h>
#include <string.h>

namespace Inertia
{
	namespace Components
	{
		namespace Storage
		{
			namespace VersionedRecord
			{
				template<typename HeaderType>
				struct IsCanonicalHeaderType
				{
					static constexpr bool Value = false;
				};

				template<>
				struct IsCanonicalHeaderType<HeaderStruct>
				{
					static constexpr bool Value = true;
				};

				template<typename storage_type, typename header_type, uint16_t format_version>
				struct Codec
				{
					using StorageType = storage_type;
					using HeaderType = header_type;

					static constexpr uint16_t FormatVersion = format_version;
					static constexpr size_t RecordSize = sizeof(StorageType);
					static constexpr size_t DataSize = RecordSize - sizeof(HeaderType);

					static_assert(IsCanonicalHeaderType<HeaderType>::Value,
						"Versioned record codecs must use the canonical versioned-record header.");
					static_assert(IsCanonicalHeaderType<decltype(((StorageType*)nullptr)->Header)>::Value,
						"A versioned record's Header member must use the canonical versioned-record header.");
					static_assert(offsetof(StorageType, Header) == 0,
						"A versioned record header must be the first member.");
					static_assert(DataSize <= UINT16_MAX,
						"Versioned record data size must fit in the header.");

					static CodecResultEnum Encode(const StorageType& record, uint8_t* destination, size_t capacity)
					{
						if (destination == nullptr) { return CodecResultEnum::InvalidArgument; }
						if (capacity < RecordSize) { return CodecResultEnum::BufferTooSmall; }

						memcpy(destination, &record, RecordSize);
						HeaderType header;
						memcpy(&header, destination, sizeof(header));
						header.Version = FormatVersion;
						header.Size = static_cast<uint16_t>(DataSize);
						memcpy(destination, &header, sizeof(header));
						SetChecksum(destination, RecordSize);
						return CodecResultEnum::Success;
					}

					static CodecResultEnum EncodeInPlace(StorageType& record)
					{
						uint8_t* bytes = reinterpret_cast<uint8_t*>(&record);
						HeaderType header;
						memcpy(&header, bytes, sizeof(header));
						header.Version = FormatVersion;
						header.Size = static_cast<uint16_t>(DataSize);
						memcpy(bytes, &header, sizeof(header));
						SetChecksum(bytes, RecordSize);
						return CodecResultEnum::Success;
					}

					static CodecResultEnum ValidateInPlace(const StorageType& record)
					{
						return ValidateBytes(reinterpret_cast<const uint8_t*>(&record), RecordSize);
					}

					static CodecResultEnum Decode(const uint8_t* source, size_t sourceSize, StorageType& record)
					{
						const CodecResultEnum validation = ValidateBytes(source, sourceSize);
						if (validation != CodecResultEnum::Success) { return validation; }
						memcpy(&record, source, RecordSize);
						return CodecResultEnum::Success;
					}

				private:
					static void SetChecksum(uint8_t* bytes, size_t length)
					{
						HeaderType header;
						memcpy(&header, bytes, sizeof(header));
						header.Checksum = ComputeChecksum(bytes + sizeof(header.Checksum), length - sizeof(header.Checksum));
						memcpy(bytes, &header, sizeof(header));
					}

					static CodecResultEnum ValidateBytes(const uint8_t* source, size_t sourceSize)
					{
						if (source == nullptr) { return CodecResultEnum::InvalidArgument; }
						if (sourceSize != RecordSize) { return CodecResultEnum::InvalidSize; }

						HeaderType header;
						memcpy(&header, source, sizeof(header));
						if (header.Version != FormatVersion) { return CodecResultEnum::UnsupportedVersion; }
						if (header.Size != DataSize) { return CodecResultEnum::InvalidSize; }
						if (header.Checksum != ComputeChecksum(
							source + sizeof(header.Checksum), RecordSize - sizeof(header.Checksum)))
						{
							return CodecResultEnum::ChecksumMismatch;
						}
						return CodecResultEnum::Success;
					}

					static uint32_t ComputeChecksum(const uint8_t* data, size_t length)
					{
						Fletcher32 checksum;
						checksum.begin();
						for (size_t index = 0; index < length; index += 2)
						{
							uint16_t word = data[index];
							if (index + 1 < length)
							{
								word |= static_cast<uint16_t>(data[index + 1]) << 8;
							}
							checksum.add(word);
						}
						return checksum.getFletcher();
					}
				};

				namespace Internal
				{
					template<typename FirstCodec, typename... OtherCodecs>
					struct MaximumRecordSize
					{
						static constexpr size_t TailSize = MaximumRecordSize<OtherCodecs...>::Value;
						static constexpr size_t Value = FirstCodec::RecordSize > TailSize
							? FirstCodec::RecordSize : TailSize;
					};

					template<typename LastCodec>
					struct MaximumRecordSize<LastCodec>
					{
						static constexpr size_t Value = LastCodec::RecordSize;
					};

					template<typename... Codecs>
					struct VersionsAreUnique;

					template<>
					struct VersionsAreUnique<> { static constexpr bool Value = true; };

					template<typename Codec, typename... OtherCodecs>
					struct VersionIsNotRepeated;

					template<typename Codec>
					struct VersionsAreUnique<Codec> { static constexpr bool Value = true; };

					template<typename Codec, typename FirstCodec, typename... OtherCodecs>
					struct VersionIsNotRepeated<Codec, FirstCodec, OtherCodecs...>
					{
						static constexpr bool Value = Codec::FormatVersion != FirstCodec::FormatVersion
							&& VersionIsNotRepeated<Codec, OtherCodecs...>::Value;
					};

					template<typename Codec>
					struct VersionIsNotRepeated<Codec> { static constexpr bool Value = true; };

					template<typename FirstCodec, typename SecondCodec, typename... OtherCodecs>
					struct VersionsAreUnique<FirstCodec, SecondCodec, OtherCodecs...>
					{
						static constexpr bool Value = VersionIsNotRepeated<FirstCodec, SecondCodec, OtherCodecs...>::Value
							&& VersionsAreUnique<SecondCodec, OtherCodecs...>::Value;
					};

					template<typename Codec> struct TypeTag {};

					template<typename FirstCodec, typename SecondCodec>
					struct AreSameType
					{
						static char Match(TypeTag<FirstCodec>*);
						static long Match(...);
						static constexpr bool Value = sizeof(Match(static_cast<TypeTag<SecondCodec>*>(nullptr))) == sizeof(char);
					};

					template<typename Codec, typename... RegisteredCodecs>
					struct IsRegistered;

					template<typename Codec>
					struct IsRegistered<Codec> { static constexpr bool Value = false; };

					template<typename Codec, typename FirstCodec, typename... RemainingCodecs>
					struct IsRegistered<Codec, FirstCodec, RemainingCodecs...>
					{
						static constexpr bool Value = AreSameType<Codec, FirstCodec>::Value
							|| IsRegistered<Codec, RemainingCodecs...>::Value;
					};

					template<typename... Codecs> struct CodecList {};
					template<typename CurrentCodec, typename SourceCodec, typename... RegisteredCodecs>
					struct MigrationChainIsValid;

					template<typename CurrentCodec, typename SourceCodec, bool Registered, bool Immediate, typename... Codecs>
					struct MigrationChainValidationStep { static constexpr bool Value = false; };

					template<typename CurrentCodec, typename SourceCodec, typename... Codecs>
					struct MigrationChainValidationStep<CurrentCodec, SourceCodec, true, true, Codecs...>
					{
						using NextCodec = typename SourceCodec::NextCodec;
						static constexpr bool Value = MigrationChainIsValid<CurrentCodec, NextCodec, Codecs...>::Value;
					};

					template<typename CurrentCodec, typename SourceCodec, typename... RegisteredCodecs>
					struct MigrationChainIsValid
					{
						using NextCodec = typename SourceCodec::NextCodec;
						static constexpr bool Value = MigrationChainValidationStep<CurrentCodec, SourceCodec,
							IsRegistered<NextCodec, CurrentCodec, RegisteredCodecs...>::Value,
							NextCodec::FormatVersion == SourceCodec::FormatVersion + 1,
							RegisteredCodecs...>::Value;
					};

					template<typename CurrentCodec, typename... Codecs>
					struct MigrationChainIsValid<CurrentCodec, CurrentCodec, Codecs...>
					{
						static constexpr bool Value = true;
					};

					template<typename CurrentCodec, typename SourceCodec, typename Registry>
					struct MigrationChainIsValidForRegistry;

					template<typename CurrentCodec, typename SourceCodec, typename... Codecs>
					struct MigrationChainIsValidForRegistry<CurrentCodec, SourceCodec, CodecList<Codecs...>>
						: MigrationChainIsValid<CurrentCodec, SourceCodec, Codecs...> {};

					template<typename CurrentCodec, typename Registry, typename... PendingCodecs>
					struct AllMigrationChainsAreValidFrom;

					template<typename CurrentCodec, typename Registry>
					struct AllMigrationChainsAreValidFrom<CurrentCodec, Registry>
					{
						static constexpr bool Value = true;
					};

					template<typename CurrentCodec, typename Registry, typename FirstCodec, typename... RemainingCodecs>
					struct AllMigrationChainsAreValidFrom<CurrentCodec, Registry, FirstCodec, RemainingCodecs...>
					{
						static constexpr bool Value = MigrationChainIsValidForRegistry<CurrentCodec, FirstCodec, Registry>::Value
							&& AllMigrationChainsAreValidFrom<CurrentCodec, Registry, RemainingCodecs...>::Value;
					};

					template<typename CurrentCodec, typename... LegacyCodecs>
					struct AllMigrationChainsAreValid
						: AllMigrationChainsAreValidFrom<CurrentCodec, CodecList<CurrentCodec, LegacyCodecs...>, LegacyCodecs...> {};

					template<typename... Codecs>
					struct AllUseCanonicalHeader;

					template<typename Codec>
					struct UsesCanonicalHeader
					{
						using StorageType = typename Codec::StorageType;
						static constexpr bool Value = IsCanonicalHeaderType<typename Codec::HeaderType>::Value
							&& IsCanonicalHeaderType<decltype(((StorageType*)nullptr)->Header)>::Value;
					};

					template<>
					struct AllUseCanonicalHeader<> { static constexpr bool Value = true; };

					template<typename FirstCodec, typename... RemainingCodecs>
					struct AllUseCanonicalHeader<FirstCodec, RemainingCodecs...>
					{
						static constexpr bool Value = UsesCanonicalHeader<FirstCodec>::Value
							&& AllUseCanonicalHeader<RemainingCodecs...>::Value;
					};

					template<typename CurrentCodec, typename SourceCodec>
					struct MigrationChain
					{
						static bool Apply(const typename SourceCodec::StorageType& source,
							typename CurrentCodec::StorageType& destination)
						{
							using NextCodec = typename SourceCodec::NextCodec;
							static_assert(NextCodec::FormatVersion == SourceCodec::FormatVersion + 1,
								"Each legacy codec must migrate to the immediately following format version.");
							typename NextCodec::StorageType next{};
							if (!SourceCodec::MigrateToNext(source, next)) { return false; }
							return MigrationChain<CurrentCodec, NextCodec>::Apply(next, destination);
						}
					};

					template<typename CurrentCodec>
					struct MigrationChain<CurrentCodec, CurrentCodec>
					{
						static bool Apply(const typename CurrentCodec::StorageType& source,
							typename CurrentCodec::StorageType& destination)
						{
							destination = source;
							return true;
						}
					};

					template<typename CurrentCodec, typename... LegacyCodecs>
					struct CodecDispatcher;

					template<typename CurrentCodec>
					struct CodecDispatcher<CurrentCodec>
					{
						static_assert(AllUseCanonicalHeader<CurrentCodec>::Value,
							"Every codec in a versioned-record migration chain must use the canonical header.");

						static CodecResultEnum Decode(const uint8_t* source, size_t sourceSize,
							typename CurrentCodec::StorageType& currentRecord)
						{
							if (source == nullptr) { return CodecResultEnum::InvalidArgument; }
							if (sourceSize < sizeof(typename CurrentCodec::HeaderType)) { return CodecResultEnum::InvalidSize; }
							typename CurrentCodec::HeaderType header;
							memcpy(&header, source, sizeof(header));
							if (header.Version != CurrentCodec::FormatVersion) { return CodecResultEnum::UnsupportedVersion; }
							return CurrentCodec::Decode(source, sourceSize, currentRecord);
						}
					};

					template<typename CurrentCodec, typename FirstCodec, typename... RemainingCodecs>
					struct CodecDispatcher<CurrentCodec, FirstCodec, RemainingCodecs...>
					{
						static_assert(AllUseCanonicalHeader<CurrentCodec, FirstCodec, RemainingCodecs...>::Value,
							"Every codec in a versioned-record migration chain must use the canonical header.");

						static CodecResultEnum Decode(const uint8_t* source, size_t sourceSize,
							typename CurrentCodec::StorageType& currentRecord)
						{
							if (source == nullptr) { return CodecResultEnum::InvalidArgument; }
							if (sourceSize < sizeof(typename CurrentCodec::HeaderType)) { return CodecResultEnum::InvalidSize; }
							typename CurrentCodec::HeaderType header;
							memcpy(&header, source, sizeof(header));
							if (header.Version == FirstCodec::FormatVersion)
							{
								typename FirstCodec::StorageType legacyRecord{};
								const CodecResultEnum validation = FirstCodec::Decode(source, sourceSize, legacyRecord);
								if (validation != CodecResultEnum::Success) { return validation; }
								return MigrationChain<CurrentCodec, FirstCodec>::Apply(legacyRecord, currentRecord)
									? CodecResultEnum::Success : CodecResultEnum::MigrationFailed;
							}
							return CodecDispatcher<CurrentCodec, RemainingCodecs...>::Decode(source, sourceSize, currentRecord);
						}
					};
				}
			}
		}
	}
}

#endif