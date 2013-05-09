#include "Tables.h"
#include "Util.h"
#include <Guid/Acpi.h>

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
	return (UINT64*) ((UINTN) xsdt + SD_HEADER_SIZE);
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

void FindAllTables(RSDP_TABLE* rsdp, TABLE_STACK* stack)
{
	UINTN i;
	UINTN acpi_version = rsdp->Revision != 0 ? 2 : 1;

	if (acpi_version == 1)
	{
		//RSDT only
		SD_HEADER* rsdt = GetRSDT(rsdp);

		UINTN table_count = 0;
		UINT32* tables = GetRSDTTables(rsdt, &table_count);

		//add each table
		for (i = 0; i < table_count; ++i)
		{
			stack->Tables[stack->TableCount] = (void*) Int32ToTable(tables[i]);
			stack->TableCount++;
		}
	}
	else //assuming 2...
	{
		//XSDT only
		SD_HEADER* xsdt = GetXSDT(rsdp);

		UINTN table_count = 0;
		UINT64* tables = GetXSDTTables(xsdt, &table_count);

		//add each table
		for (i = 0; i < table_count; ++i)
		{
			stack->Tables[stack->TableCount] = (void*) Int64ToTable(tables[i]);
			stack->TableCount++;
		}
	}
}

void PatchTables(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* systab,
		void* slic_data, UINTN slic_size)
{
	EFI_GUID rsdp_2_guid = EFI_ACPI_20_TABLE_GUID;
	EFI_GUID rsdp_1_guid = EFI_ACPI_TABLE_GUID;
	EFI_STATUS res;

	CHAR8 set_OEMID[6];
	CHAR8 set_OEMTableID[8];
	UINT32 set_OEMRevision = 0;
	CHAR8 set_CreatorID[4];
	CHAR8 set_CreatorRevision[4];

	// Get RSDP and revision

	RSDP_TABLE* rsdp;
	res = EfiGetSystemConfigurationTable(&rsdp_2_guid, (void**) &rsdp);

	if (res)
	{
		res = EfiGetSystemConfigurationTable(&rsdp_1_guid, (void**) &rsdp);

		if (res)
		{
			//cant find it...
			ErrorPrint(L"Can't find RSDP!\r\n");
			return;
		}
	}

	UINTN acpi_version = rsdp->Revision != 0 ? 2 : 1;

	//Get all tables
	TABLE_STACK old_tables;
	old_tables.TableCount = 0;

	FindAllTables(rsdp, &old_tables);

	//Now make new tables
	TABLE_STACK new_tables;
	new_tables.TableCount = 0;

	//copy, except for SLIC tables
	UINTN i;
	for (i = 0; i < old_tables.TableCount; ++i)
	{
		SD_HEADER* current = (SD_HEADER*) old_tables.Tables[i];
		if (IsTableType(current, "SLIC") || IsTableType(current, "MSDM"))
		{
			continue; //skip SLIC
		}

		UINTN table_size = current->Length;

		//allocate new
		SD_HEADER* new_table = AllocateACPI(table_size, systab);

		//copy it
		systab->BootServices->CopyMem((void*) new_table, (void*) current,
				table_size);

		//add to stack
		new_tables.Tables[new_tables.TableCount] = (void*) new_table;
		new_tables.TableCount++;

		//mangle existing table header so it doesn't get found accidentally
		current->Signature[0] = 0;
		current->Signature[1] = 0;
		current->Signature[2] = 0;
		current->Signature[3] = 0;
	}

	SD_HEADER* new_slic = AllocateACPI(slic_size, systab);
	systab->BootServices->CopyMem((void*) new_slic, (void*) slic_data,
			slic_size);

	//get new oem stuff
	systab->BootServices->CopyMem((void*) set_OEMID, (void*) new_slic->OEMID,
			6);
	systab->BootServices->CopyMem((void*) set_OEMTableID,
			(void*) new_slic->OEMTableID, 8);
	set_OEMRevision = new_slic->OEMRevision;
	systab->BootServices->CopyMem((void*) set_CreatorID,
			(void*) new_slic->CreatorID, 4);
	systab->BootServices->CopyMem((void*) set_CreatorRevision,
			(void*) new_slic->CreatorRevision, 4);

	//add new slic
	new_tables.Tables[new_tables.TableCount] = (void*) new_slic;
	new_tables.TableCount++;

	//change FACP table OEM id stuff
	for (i = 0; i < new_tables.TableCount; ++i)
	{
		SD_HEADER* curr = (SD_HEADER*) new_tables.Tables[i];
		if (!IsTableType(curr, "FACP"))
		{
			continue;
		}

		systab->BootServices->CopyMem((void*) &curr->OEMID, set_OEMID, 6);
		systab->BootServices->CopyMem((void*) &curr->OEMTableID, set_OEMTableID,
				8);
		curr->OEMRevision = set_OEMRevision;
		systab->BootServices->CopyMem((void*) &curr->CreatorID, set_CreatorID,
				4);
		systab->BootServices->CopyMem((void*) &curr->CreatorRevision,
				set_CreatorRevision, 4);

		//new checksum
		curr->Checksum = GetChecksum((void*) curr, curr->Length, 9);
	}

	//Now create new RSDT and XSDT tables

	//RSDT
	//figure out RSDT length
	UINTN rsdt_length = SD_HEADER_SIZE + sizeof(UINT32) * new_tables.TableCount;

	SD_HEADER* rsdt = (SD_HEADER*) AllocateACPI(rsdt_length, systab);

	rsdt->Signature[0] = 'R';
	rsdt->Signature[1] = 'S';
	rsdt->Signature[2] = 'D';
	rsdt->Signature[3] = 'T';
	rsdt->Length = rsdt_length;
	rsdt->Revision = 1;
	//skip checksum, offset 9
	systab->BootServices->CopyMem((void*) &rsdt->OEMID, set_OEMID, 6);
	systab->BootServices->CopyMem((void*) &rsdt->OEMTableID, set_OEMTableID, 8);
	rsdt->OEMRevision = set_OEMRevision;
	systab->BootServices->CopyMem((void*) &rsdt->CreatorID, set_CreatorID, 4);
	systab->BootServices->CopyMem((void*) &rsdt->CreatorRevision,
			set_CreatorRevision, 4);

	//append tables
	for (i = 0; i < new_tables.TableCount; ++i)
	{
		//get pointer
		UINT32* ptr = (UINT32*) ((UINTN) rsdt + SD_HEADER_SIZE
				+ sizeof(UINT32) * i);
		*ptr = (UINT32) (UINTN) new_tables.Tables[i];
	}

	//checksum
	rsdt->Checksum = GetChecksum((void*) rsdt, rsdt_length, 9);

	//done, now do XSDT
	SD_HEADER* xsdt = NULL;
	UINTN xsdt_length = 0;

	if (acpi_version == 2)
	{
		xsdt_length = SD_HEADER_SIZE + sizeof(UINT64) * new_tables.TableCount;
		xsdt = (SD_HEADER*) AllocateACPI(xsdt_length, systab);

		xsdt->Signature[0] = 'X';
		xsdt->Signature[1] = 'S';
		xsdt->Signature[2] = 'D';
		xsdt->Signature[3] = 'T';
		xsdt->Length = xsdt_length;
		xsdt->Revision = 1;
		//skip checksum, offset 9
		systab->BootServices->CopyMem((void*) &xsdt->OEMID, set_OEMID, 6);
		systab->BootServices->CopyMem((void*) &xsdt->OEMTableID, set_OEMTableID,
				8);
		xsdt->OEMRevision = set_OEMRevision;
		systab->BootServices->CopyMem((void*) &xsdt->CreatorID, set_CreatorID,
				4);
		systab->BootServices->CopyMem((void*) &xsdt->CreatorRevision,
				set_CreatorRevision, 4);

		//append tables
		for (i = 0; i < new_tables.TableCount; ++i)
		{
			//get pointer
			UINT64* ptr = (UINT64*) ((UINTN) xsdt + SD_HEADER_SIZE
					+ sizeof(UINT64) * i);
			*ptr = (UINT64) new_tables.Tables[i];
		}

		//checksum
		xsdt->Checksum = GetChecksum((void*) xsdt, xsdt_length, 9);
	}

	//done. Now set RSDP pointers
	SD_HEADER* old_rsdt = GetRSDT(rsdp);
	old_rsdt->Signature[0] = 0;
	old_rsdt->Signature[1] = 0;
	old_rsdt->Signature[2] = 0;
	old_rsdt->Signature[3] = 0;

	rsdp->RSDT = (UINT32) (UINTN) rsdt;
	rsdp->Checksum = GetChecksum((void*) rsdt, 20, 8);

	if (acpi_version == 2)
	{
		SD_HEADER* old_xsdt = GetXSDT(rsdp);
		old_xsdt->Signature[0] = 0;
		old_xsdt->Signature[1] = 0;
		old_xsdt->Signature[2] = 0;
		old_xsdt->Signature[3] = 0;
		rsdp->XSDT = (UINT64) (UINTN) xsdt;
		rsdp->ExtendedChecksum = GetChecksum((void*) rsdt, rsdt->Length, 32);
	}

	//Complete.
}
