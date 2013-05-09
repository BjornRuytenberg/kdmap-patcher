#include "Tables.h"
#include "Util.h"

SD_HEADER* Int32ToTable(UINT32 i)
{
	return (SD_HEADER*) (UINTN) i;
}

SD_HEADER* Int64ToTable(UINT64 i)
{
	return (SD_HEADER*) (UINTN) i;
}

SD_HEADER* GetRSDT(RSDP_TABLE* rsdp)
{
	return Int32ToTable(rsdp->RSDT);
}

SD_HEADER* GetXSDT(RSDP_TABLE* rsdp)
{
	return Int64ToTable(rsdp->XSDT);
}

UINT32* GetRSDTTables(SD_HEADER* rsdt, UINTN* count)
{
	*count = (rsdt->Length - SD_HEADER_SIZE) / sizeof(UINT32);
	return (UINT32*) (rsdt + SD_HEADER_SIZE);
}

UINT64* GetXSDTTables(SD_HEADER* xsdt, UINTN* count)
{
	*count = (xsdt->Length - SD_HEADER_SIZE) / sizeof(UINT64);
	return (UINT64*) (xsdt + SD_HEADER_SIZE);
}

UINT8 GetChecksum(void* data, UINTN count, UINTN exclude)
{
	UINT8 sum = 0;
	UINTN i;
	for (i = 0; i < count; ++i)
	{
		if (i == exclude)
		{
			continue;
		}

		UINT8 val = *((UINT8*) (data + i));
		sum = (sum + val) % 256;
	}

	return (256 - sum) % 256;
}

BOOLEAN IsTableType(SD_HEADER* table, CHAR8* sig)
{
	return table->Signature[0] == sig[0] && table->Signature[1] == sig[1]
			&& table->Signature[2] == sig[2] && table->Signature[3] == sig[3];
}

