#ifndef _INERTIA_DRIVERS_STORAGE_FLASH_MAPLE_INTERNAL_FLASH_h
#define _INERTIA_DRIVERS_STORAGE_FLASH_MAPLE_INTERNAL_FLASH_h

#if defined(ARDUINO_ARCH_STM32F1) && defined(__STM32F1__) && !defined(STM32_CORE_VERSION)

#include <wirish.h>
#include <flash_stm32.h>

extern "C" uint32_t _lm_rom_img_cfgp;

namespace Inertia
{
	namespace Drivers
	{
		namespace Storage
		{
			namespace Flash
			{
				namespace Stm32
				{
					namespace Maple
					{
						class InternalFlash
						{
						public:
							enum
							{
								PageSizeBytes = 1024,
								PhysicalFlashSizeBytes = 128 * 1024
							};

							static uint32_t GetFlashBaseAddress()
							{
								return 0x08000000UL;
							}

							static uint16_t ReadHalfWord(const uint32_t address)
							{
								return *reinterpret_cast<volatile uint16_t*>(address);
							}

							static bool ProgramHalfWord(const uint32_t address, const uint16_t value)
							{
								if ((address & 1) != 0 || !IsImageFreeRange(address, address + 2))
									return false;

								FLASH_Unlock();
								const FLASH_Status status = FLASH_ProgramHalfWord(address, value);
								FLASH_Lock();
								return status == FLASH_COMPLETE;
							}

							static bool ErasePage(const uint32_t address)
							{
								if (address % PageSizeBytes != 0
									|| !IsStorageRegionAvailable(address, address + PageSizeBytes))
									return false;

								FLASH_Unlock();
								const FLASH_Status status = FLASH_ErasePage(address);
								FLASH_Lock();
								return status == FLASH_COMPLETE;
							}

							static bool IsStorageRegionAvailable(const uint32_t startAddress, const uint32_t endAddress)
							{
								const uint32_t physicalFlashSize = GetPhysicalFlashSizeBytes();
								if (physicalFlashSize == 0)
									return false;

								return startAddress >= GetFlashBaseAddress()
									&& endAddress > startAddress
									&& endAddress <= GetFlashBaseAddress() + physicalFlashSize
									&& startAddress >= GetFlashImageEndAddress()
									&& (startAddress - GetFlashBaseAddress()) % PageSizeBytes == 0
									&& (endAddress - startAddress) % PageSizeBytes == 0;
							}

							/// <summary>Find a page-aligned region at the end of flash, rounded up to whole pages and rejected if it overlaps the program.</summary>
							static bool TryGetStorageRegion(const uint32_t requiredSizeBytes, uint32_t& startAddress)
							{
								startAddress = 0;
								if (requiredSizeBytes == 0 || requiredSizeBytes > PhysicalFlashSizeBytes)
									return false;

								const uint32_t pageCount = (requiredSizeBytes + PageSizeBytes - 1) / PageSizeBytes;
								const uint32_t regionSizeBytes = pageCount * PageSizeBytes;
								const uint32_t physicalFlashSize = GetPhysicalFlashSizeBytes();
								if (physicalFlashSize == 0 || regionSizeBytes > physicalFlashSize)
									return false;

								const uint32_t regionEndAddress = GetFlashBaseAddress() + physicalFlashSize;
								const uint32_t candidateStartAddress = regionEndAddress - regionSizeBytes;
								if (!IsStorageRegionAvailable(candidateStartAddress, regionEndAddress))
									return false;

								startAddress = candidateStartAddress;
								return true;
							}

						private:
							static uint32_t GetFlashImageEndAddress()
							{
								return reinterpret_cast<uint32_t>(&_lm_rom_img_cfgp) + sizeof(uint32_t);
							}

							static uint32_t GetPhysicalFlashSizeBytes()
							{
								const uint16_t sizeInKilobytes = *reinterpret_cast<volatile uint16_t*>(0x1FFFF7E0UL);
								if (sizeInKilobytes == 0 || sizeInKilobytes > PhysicalFlashSizeBytes / 1024)
									return 0;
								return uint32_t(sizeInKilobytes) * 1024;
							}

							static bool IsImageFreeRange(const uint32_t startAddress, const uint32_t endAddress)
							{
								const uint32_t physicalFlashSize = GetPhysicalFlashSizeBytes();
								if (physicalFlashSize == 0)
									return false;

								return startAddress >= GetFlashBaseAddress()
									&& endAddress > startAddress
									&& endAddress <= GetFlashBaseAddress() + physicalFlashSize
									&& startAddress >= GetFlashImageEndAddress();
							}
						};
					}
				}
			}
		}
	}
}

#endif
#endif
