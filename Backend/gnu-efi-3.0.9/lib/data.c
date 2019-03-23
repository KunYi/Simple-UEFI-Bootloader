/*++

Copyright (c) 1998  Intel Corporation

Module Name:

    data.c

Abstract:

    EFI library global data



Revision History

--*/

#include "lib.h"

//
// LibInitialized - TRUE once InitializeLib() is called for the first time
//

BOOLEAN  LibInitialized = FALSE;

//
// ImageHandle - Current ImageHandle, as passed to InitializeLib
//
EFI_HANDLE LibImageHandle;

//
// ST - pointer to the EFI system table
//

EFI_SYSTEM_TABLE        *ST;

//
// BS - pointer to the boot services table
//

EFI_BOOT_SERVICES       *BS;


//
// Default pool allocation type
//

EFI_MEMORY_TYPE PoolAllocationType = EfiBootServicesData;

//
// Unicode collation functions that are in use
//

EFI_UNICODE_COLLATION_INTERFACE   LibStubUnicodeInterface = {
    LibStubStriCmp,
    LibStubMetaiMatch,
    LibStubStrLwrUpr,
    LibStubStrLwrUpr,
    NULL,   // FatToStr
    NULL,   // StrToFat
    NULL    // SupportedLanguages
};

EFI_UNICODE_COLLATION_INTERFACE   *UnicodeInterface = &LibStubUnicodeInterface;

//
// Root device path
//

EFI_DEVICE_PATH RootDevicePath[] = {
   {END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, {END_DEVICE_PATH_LENGTH,0}}
};

EFI_DEVICE_PATH EndDevicePath[] = {
   {END_DEVICE_PATH_TYPE, END_ENTIRE_DEVICE_PATH_SUBTYPE, {END_DEVICE_PATH_LENGTH, 0}}
};

EFI_DEVICE_PATH EndInstanceDevicePath[] = {
   {END_DEVICE_PATH_TYPE, END_INSTANCE_DEVICE_PATH_SUBTYPE, {END_DEVICE_PATH_LENGTH, 0}}
};


//
// EFI IDs
//

EFI_GUID gEfiGlobalVariableGuid = EFI_GLOBAL_VARIABLE;
EFI_GUID NullGuid = { 0,0,0,{0,0,0,0,0,0,0,0} };

// GNU-EFI was missing these, so I added them from EDK II for GUID matching purposes.
// They probably won't work for actual use (except EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL_GUID).

#define EFI_STANDARD_ERROR_DEVICE_GUID \
    { 0xd3b36f2d, 0xd551, 0x11d4, {0x9a, 0x46, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d} }

EFI_GUID gEfiStandardErrorDeviceGuid = EFI_STANDARD_ERROR_DEVICE_GUID;

#define EFI_UGA_DRAW_PROTOCOL_GUID \
    { 0x982c298b, 0xf4fa, 0x41cb, {0xb8, 0x38, 0x77, 0xaa, 0x68, 0x8f, 0xb8, 0x39} }

EFI_GUID gEfiUgaDrawProtocolGuid = EFI_UGA_DRAW_PROTOCOL_GUID;

#define EFI_CONSOLE_OUT_DEVICE_GUID \
    { 0xd3b36f2c, 0xd551, 0x11d4, {0x9a, 0x46, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d} }

EFI_GUID gEfiConsoleOutDeviceGuid = EFI_CONSOLE_OUT_DEVICE_GUID;

#define EFI_CONSOLE_IN_DEVICE_GUID \
    { 0xd3b36f2b, 0xd551, 0x11d4, {0x9a, 0x46, 0x00, 0x90, 0x27, 0x3f, 0xc1, 0x4d} }

EFI_GUID gEfiConsoleInDeviceGuid = EFI_CONSOLE_IN_DEVICE_GUID;

#define EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL_GUID \
    { 0x5c198761, 0x16a8, 0x4e69, {0x97, 0x2c, 0x89, 0xd6, 0x79, 0x54, 0xf8, 0x1d} }

EFI_GUID gEfiDriverSupportedEfiVersionProtocolGuid = EFI_DRIVER_SUPPORTED_EFI_VERSION_PROTOCOL_GUID;

#define EFI_DRIVER_DIAGNOSTICS_PROTOCOL_GUID \
    { 0x0784924F, 0xE296, 0x11D4, {0x9A, 0x49, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D} }

EFI_GUID gEfiDriverDiagnosticsProtocolGuid = EFI_DRIVER_DIAGNOSTICS_PROTOCOL_GUID;

#define EFI_DRIVER_DIAGNOSTICS2_PROTOCOL_GUID \
    { 0x4D330321, 0x025F, 0x4AAC, {0x90, 0xD8, 0x5E, 0xD9, 0x00, 0x17, 0x3B, 0x63} }

EFI_GUID gEfiDriverDiagnostics2ProtocolGuid = EFI_DRIVER_DIAGNOSTICS2_PROTOCOL_GUID;

#define EFI_DRIVER_CONFIGURATION_PROTOCOL_GUID \
    { 0x107A772B, 0xD5E1, 0x11D4, {0x9A, 0x46, 0x00, 0x90, 0x27, 0x3F, 0xC1, 0x4D} }

EFI_GUID gEfiDriverConfigurationProtocolGuid = EFI_DRIVER_CONFIGURATION_PROTOCOL_GUID;

#define EFI_USB_POLICY_PROTOCOL_GUID \
    { 0x5859cb76, 0x6bef, 0x468a, {0xbe, 0x2d, 0xb3, 0xdd, 0x1a, 0x27, 0xf0, 0x12} }

EFI_GUID gEfiUsbPolicyProtocolGuid = EFI_USB_POLICY_PROTOCOL_GUID;

#define EFI_USB_TIMING_POLICY_PROTOCOL_GUID \
    { 0x89e3c1dc, 0xb5e3, 0x4d34, {0xae, 0xad, 0xdd, 0x7e, 0xb2, 0x82, 0x8c, 0x18} }

EFI_GUID gEfiUsbTimingPolicyProtocolGuid = EFI_USB_TIMING_POLICY_PROTOCOL_GUID;

#define EFI_USB_IO_PROTOCOL_GUID \
    { 0x2B2F68D6, 0x0CD2, 0x44CF, {0x8E, 0x8B, 0xBB, 0xA2, 0x0B, 0x1B, 0x5B, 0x75} }

EFI_GUID gEfiUsbIoProtocolGuid = EFI_USB_IO_PROTOCOL_GUID;

#define EFI_USB2_HC_PROTOCOL_GUID \
    { 0x3e745226, 0x9818, 0x45b6, {0xa2, 0xac, 0xd7, 0xcd, 0x0e, 0x8b, 0xa2, 0xbc} }

EFI_GUID gEfiUsb2HcProtocolGuid = EFI_USB2_HC_PROTOCOL_GUID;

#define EFI_USB_HC_PROTOCOL_GUID \
    { 0xf5089266, 0x1aa0, 0x4953, {0x97, 0xd8, 0x56, 0x2f, 0x8a, 0x73, 0xb5, 0x19} }

EFI_GUID gEfiUsbHcProtocolGuid = EFI_USB_HC_PROTOCOL_GUID;

#define EFI_DATA_HUB_PROTOCOL_GUID \
    { 0xae80d021, 0x618e, 0x11d4, {0xbc, 0xd7, 0x00, 0x80, 0xc7, 0x3c, 0x88, 0x81} }

EFI_GUID gEfiDataHubProtocolGuid = EFI_DATA_HUB_PROTOCOL_GUID;

#define EFI_CONSOLE_IN_DEVICES_STARTED_PROTOCOL_GUID \
    { 0x2df1e051, 0x906d, 0x4eff, {0x86, 0x9d, 0x24, 0xe6, 0x53, 0x78, 0xfb, 0x9e} }

EFI_GUID gEfiConsoleInDevicesStartedGuid = EFI_CONSOLE_IN_DEVICES_STARTED_PROTOCOL_GUID;

#define EFI_CONSOLE_OUT_DEVICES_STARTED_PROTOCOL_GUID \
    { 0xef9a3971, 0xc1a0, 0x4a93, {0xbd, 0x40, 0x5a, 0xa1, 0x65, 0xf2, 0xdc, 0x3a} }

EFI_GUID gEfiConsoleOutDevicesStartedGuid = EFI_CONSOLE_OUT_DEVICES_STARTED_PROTOCOL_GUID;

#define EFI_PLATFORM_IDE_PROTOCOL_GUID \
    { 0x6737f69b, 0xb8cc, 0x45bc, {0x93, 0x27, 0xcc, 0xf5, 0xee, 0xf7, 0x0c, 0xde} }

EFI_GUID gEfiPlatformIDEProtocolGuid = EFI_PLATFORM_IDE_PROTOCOL_GUID;

#define EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID \
    { 0xdd9e7534, 0x7762, 0x4698, {0x8c, 0x14, 0xf5, 0x85, 0x17, 0xa6, 0x25, 0xaa} }

EFI_GUID gEfiSimpleTextInExProtocolGuid = EFI_SIMPLE_TEXT_INPUT_EX_PROTOCOL_GUID;

#define EFI_DISK_INFO_PROTOCOL_GUID \
    { 0xd432a67f, 0x14dc, 0x484b, {0xb3, 0xbb, 0x3f, 0x02, 0x91, 0x84, 0x93, 0x27} }

EFI_GUID gEfiDiskInfoProtocolGuid = EFI_DISK_INFO_PROTOCOL_GUID;

#define EFI_SCSI_IO_PROTOCOL_GUID \
    { 0x932f47e6, 0x2362, 0x4002, {0x80, 0x3e, 0x3c, 0xd5, 0x4b, 0x13, 0x8f, 0x85} }

EFI_GUID gEfiScsiIoProtocolGuid = EFI_SCSI_IO_PROTOCOL_GUID;

#define EFI_EXT_SCSI_PASS_THRU_PROTOCOL_GUID \
    { 0x143b7632, 0xb81b, 0x4cb7, {0xab, 0xd3, 0xb6, 0x25, 0xa5, 0xb9, 0xbf, 0xfe} }

EFI_GUID gEfiExtScsiPassThruProtocolGuid = EFI_EXT_SCSI_PASS_THRU_PROTOCOL_GUID;

#define EFI_FIRMWARE_VOLUME2_PROTOCOL_GUID \
    { 0x220e73b6, 0x6bdb, 0x4413, {0x84, 0x05, 0xb9, 0x74, 0xb1, 0x08, 0x61, 0x9a} }

EFI_GUID gEfiFirmwareVolume2ProtocolGuid = EFI_FIRMWARE_VOLUME2_PROTOCOL_GUID;

#define EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL_GUID \
    { 0x8f644fa9, 0xe850, 0x4db1, {0x9c, 0xe2, 0x0b, 0x44, 0x69, 0x8e, 0x8d, 0xa4 } }

EFI_GUID gEfiFirmwareVolumeBlockProtocolGuid = EFI_FIRMWARE_VOLUME_BLOCK_PROTOCOL_GUID;

#define EFI_FIRMWARE_VOLUME_DISPATCH_PROTOCOL_GUID \
    { 0x7aa35a69, 0x506c, 0x444f, {0xa7, 0xaf, 0x69, 0x4b, 0xf5, 0x6f, 0x71, 0xc8} }

EFI_GUID gEfiFirmwareVolumeDispatchProtocolGuid = EFI_FIRMWARE_VOLUME_DISPATCH_PROTOCOL_GUID;

#define EFI_LOAD_FILE2_PROTOCOL_GUID \
    { 0x4006c0c1, 0xfcb3, 0x403e, {0x99, 0x6d, 0x4a, 0x6c, 0x87, 0x24, 0xe0, 0x6d} }

EFI_GUID gEfiLoadFile2ProtocolGuid = EFI_LOAD_FILE2_PROTOCOL_GUID;

#define EFI_SIO_PROTOCOL_GUID \
    { 0x215fdd18, 0xbd50, 0x4feb, {0x89, 0x0b, 0x58, 0xca, 0x0b, 0x47, 0x39, 0xe9} }

EFI_GUID gEfiSioProtocolGuid = EFI_SIO_PROTOCOL_GUID; // Simple IO, appears to be mainly used for PS/2 devices like mice

#define EFI_IDE_CONTROLLER_INIT_PROTOCOL_GUID \
    { 0xa1e37052, 0x80d9, 0x4e65, {0xa3, 0x17, 0x3e, 0x9a, 0x55, 0xc4, 0x3e, 0xc9} }

EFI_GUID gEfiIdeControllerInitProtocolGuid = EFI_IDE_CONTROLLER_INIT_PROTOCOL_GUID;

#define EFI_STORAGE_SECURITY_COMMAND_PROTOCOL_GUID \
    { 0xc88b0b6d, 0x0dfc, 0x49a7, {0x9c, 0xb4, 0x49, 0x07, 0x4b, 0x4c, 0x3a, 0x78} }

EFI_GUID gEfiStorageSecurityCommandProtocolGuid = EFI_STORAGE_SECURITY_COMMAND_PROTOCOL_GUID;

#define EFI_HII_CONFIG_ACCESS_PROTOCOL_GUID \
    { 0x330d4706, 0xf2a0, 0x4e4f, {0xa3, 0x69, 0xb6, 0x6f, 0xa8, 0xd5, 0x43, 0x85} }

EFI_GUID gEfiHiiConfigAccessProtocolGuid = EFI_HII_CONFIG_ACCESS_PROTOCOL_GUID;

// There's also a different gEfiNetworkInterfaceIdentifierProtocolGuid, which looks like it's actually implemented in GNU-EFI
#define EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_GUID_31 \
    { 0x1ACED566, 0x76ED, 0x4218, {0xBC, 0x81, 0x76, 0x7F, 0x1F, 0x97, 0x7A, 0x89} }

EFI_GUID gEfiNetworkInterfaceIdentifierProtocolGuid_31 = EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_GUID_31;



#define AMI_EFIKEYCODE_PROTOCOL_GUID \
    { 0x0adfb62d, 0xff74, 0x484c, {0x89, 0x44, 0xf8, 0x5c, 0x4b, 0xea, 0x87, 0xa8} }

EFI_GUID gAmiEfikeycodeProtocolGuid = AMI_EFIKEYCODE_PROTOCOL_GUID;

#define HOT_PLUG_DEVICE_GUID \
    { 0x220ac432, 0x1d43, 0x49e5, {0xa7, 0x4f, 0x4c, 0x9d, 0xa6, 0x7a, 0xd2, 0x3b} }

EFI_GUID gHotPlugDeviceGuid = HOT_PLUG_DEVICE_GUID;



#define HDD_UNLOCKED_GUID \
    { 0x1fd29be6, 0x70d0, 0x42a4, {0xa6, 0xe7, 0xe5, 0xd1, 0x0e, 0x6a, 0xc3, 0x76} }

EFI_GUID gHddUnlockedGuid = HDD_UNLOCKED_GUID;

#define HDD_SECURITY_END_PROTOCOL_GUID \
    { 0xad77ae29, 0x4c20, 0x4fdd, {0x85, 0x04, 0x81, 0x76, 0x61, 0x9b, 0x67, 0x6a} }

EFI_GUID gHddSecurityEndProtocolGuid = HDD_SECURITY_END_PROTOCOL_GUID;

#define AHCI_BUS_INIT_PROTOCOL_GUID \
    { 0xB2FA4764, 0x3B6E, 0x43D3, {0x91, 0xDF, 0x87, 0xD1, 0x5A, 0x3E, 0x56, 0x68} }

EFI_GUID gAhciBusInitProtocolGuid = AHCI_BUS_INIT_PROTOCOL_GUID;

#define PCH_SATA_CONTROLLER_DRIVER_GUID \
    { 0xbb929da9, 0x68f7, 0x4035, {0xb2, 0x2c, 0xa3, 0xbb, 0x3f, 0x23, 0xda, 0x55} }

EFI_GUID gPchSataControllerDriverGuid = PCH_SATA_CONTROLLER_DRIVER_GUID;



// Don't know if this is right, but it seems to be unique to Intel GOP drivers. Maybe it exists for finding it in the handle database?
#define INTEL_GOP_ID_GUID \
    { 0x39487c79, 0x236d, 0x4666, {0x87, 0xe5, 0x09, 0x54, 0x7c, 0xaa, 0xe1, 0xbc} }

EFI_GUID gIntelGopGuid = INTEL_GOP_ID_GUID;

// End matching additions

//
// Protocol IDs
//

EFI_GUID gEfiDevicePathProtocolGuid                 = EFI_DEVICE_PATH_PROTOCOL_GUID;
EFI_GUID gEfiDevicePathToTextProtocolGuid           = EFI_DEVICE_PATH_TO_TEXT_PROTOCOL_GUID;
EFI_GUID gEfiDevicePathFromTextProtocolGuid         = EFI_DEVICE_PATH_FROM_TEXT_PROTOCOL_GUID;
EFI_GUID gEfiLoadedImageDevicePathProtocolGuid      = EFI_LOADED_IMAGE_DEVICE_PATH_PROTOCOL_GUID;
EFI_GUID gEfiLoadedImageProtocolGuid                = EFI_LOADED_IMAGE_PROTOCOL_GUID;
EFI_GUID gEfiSimpleTextInProtocolGuid               = EFI_SIMPLE_TEXT_INPUT_PROTOCOL_GUID;
EFI_GUID gEfiSimpleTextOutProtocolGuid              = EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL_GUID;
EFI_GUID gEfiBlockIoProtocolGuid                    = EFI_BLOCK_IO_PROTOCOL_GUID;
EFI_GUID gEfiBlockIo2ProtocolGuid                   = EFI_BLOCK_IO2_PROTOCOL_GUID;
EFI_GUID gEfiDiskIoProtocolGuid                     = EFI_DISK_IO_PROTOCOL_GUID;
EFI_GUID gEfiDiskIo2ProtocolGuid                    = EFI_DISK_IO2_PROTOCOL_GUID;
EFI_GUID gEfiSimpleFileSystemProtocolGuid           = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
EFI_GUID gEfiLoadFileProtocolGuid                   = EFI_LOAD_FILE_PROTOCOL_GUID;
EFI_GUID gEfiDeviceIoProtocolGuid                   = EFI_DEVICE_IO_PROTOCOL_GUID;
EFI_GUID gEfiUnicodeCollationProtocolGuid           = EFI_UNICODE_COLLATION_PROTOCOL_GUID;
EFI_GUID gEfiSerialIoProtocolGuid                   = EFI_SERIAL_IO_PROTOCOL_GUID;
EFI_GUID gEfiSimpleNetworkProtocolGuid              = EFI_SIMPLE_NETWORK_PROTOCOL_GUID;
EFI_GUID gEfiPxeBaseCodeProtocolGuid                = EFI_PXE_BASE_CODE_PROTOCOL_GUID;
EFI_GUID gEfiPxeBaseCodeCallbackProtocolGuid        = EFI_PXE_BASE_CODE_CALLBACK_PROTOCOL_GUID;
EFI_GUID gEfiNetworkInterfaceIdentifierProtocolGuid = EFI_NETWORK_INTERFACE_IDENTIFIER_PROTOCOL_GUID;
EFI_GUID gEFiUiInterfaceProtocolGuid                = EFI_UI_INTERFACE_PROTOCOL_GUID;
EFI_GUID gEfiPciIoProtocolGuid                      = EFI_PCI_IO_PROTOCOL_GUID;
EFI_GUID gEfiPciRootBridgeIoProtocolGuid            = EFI_PCI_ROOT_BRIDGE_IO_PROTOCOL_GUID;
EFI_GUID gEfiDriverBindingProtocolGuid              = EFI_DRIVER_BINDING_PROTOCOL_GUID;
EFI_GUID gEfiComponentNameProtocolGuid              = EFI_COMPONENT_NAME_PROTOCOL_GUID;
EFI_GUID gEfiComponentName2ProtocolGuid             = EFI_COMPONENT_NAME2_PROTOCOL_GUID;
EFI_GUID gEfiHashProtocolGuid                       = EFI_HASH_PROTOCOL_GUID;
EFI_GUID gEfiPlatformDriverOverrideProtocolGuid     = EFI_PLATFORM_DRIVER_OVERRIDE_PROTOCOL_GUID;
EFI_GUID gEfiBusSpecificDriverOverrideProtocolGuid  = EFI_BUS_SPECIFIC_DRIVER_OVERRIDE_PROTOCOL_GUID;
EFI_GUID gEfiDriverFamilyOverrideProtocolGuid       = EFI_DRIVER_FAMILY_OVERRIDE_PROTOCOL_GUID;
EFI_GUID gEfiEbcProtocolGuid                        = EFI_EBC_PROTOCOL_GUID;

//
// File system information IDs
//

EFI_GUID gEfiFileInfoGuid                           = EFI_FILE_INFO_ID;
EFI_GUID gEfiFileSystemInfoGuid                     = EFI_FILE_SYSTEM_INFO_ID;
EFI_GUID gEfiFileSystemVolumeLabelInfoIdGuid        = EFI_FILE_SYSTEM_VOLUME_LABEL_INFO_ID;

//
// Reference implementation public protocol IDs
//

EFI_GUID InternalShellProtocol = INTERNAL_SHELL_GUID;
EFI_GUID VariableStoreProtocol = VARIABLE_STORE_PROTOCOL;
EFI_GUID LegacyBootProtocol = LEGACY_BOOT_PROTOCOL;
EFI_GUID VgaClassProtocol = VGA_CLASS_DRIVER_PROTOCOL;

EFI_GUID TextOutSpliterProtocol = TEXT_OUT_SPLITER_PROTOCOL;
EFI_GUID ErrorOutSpliterProtocol = ERROR_OUT_SPLITER_PROTOCOL;
EFI_GUID TextInSpliterProtocol = TEXT_IN_SPLITER_PROTOCOL;
/* Added for GOP support */
EFI_GUID gEfiGraphicsOutputProtocolGuid             = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID;
EFI_GUID gEfiEdidDiscoveredProtocolGuid             = EFI_EDID_DISCOVERED_PROTOCOL_GUID;
EFI_GUID gEfiEdidActiveProtocolGuid                 = EFI_EDID_ACTIVE_PROTOCOL_GUID;
EFI_GUID gEfiEdidOverrideProtocolGuid               = EFI_EDID_OVERRIDE_PROTOCOL_GUID;

EFI_GUID AdapterDebugProtocol = ADAPTER_DEBUG_PROTOCOL;

//
// Device path media protocol IDs
//
EFI_GUID gEfiPcAnsiGuid                             = EFI_PC_ANSI_GUID;
EFI_GUID gEfiVT100Guid                              = EFI_VT_100_GUID;
EFI_GUID gEfiVT100PlusGuid                          = EFI_VT_100_PLUS_GUID;
EFI_GUID gEfiVTUTF8Guid                             = EFI_VT_UTF8_GUID;

//
// EFI GPT Partition Type GUIDs
//
EFI_GUID EfiPartTypeSystemPartitionGuid = EFI_PART_TYPE_EFI_SYSTEM_PART_GUID;
EFI_GUID EfiPartTypeLegacyMbrGuid = EFI_PART_TYPE_LEGACY_MBR_GUID;


//
// Reference implementation Vendor Device Path Guids
//
EFI_GUID UnknownDevice      = UNKNOWN_DEVICE_GUID;

//
// Configuration Table GUIDs
//

EFI_GUID MpsTableGuid             = MPS_TABLE_GUID;
EFI_GUID AcpiTableGuid            = ACPI_TABLE_GUID;
EFI_GUID Acpi20TableGuid          = ACPI_20_TABLE_GUID;
EFI_GUID SMBIOSTableGuid          = SMBIOS_TABLE_GUID;
EFI_GUID SalSystemTableGuid       = SAL_SYSTEM_TABLE_GUID;

//
// Network protocol GUIDs
//
EFI_GUID Ip4ServiceBindingProtocol = EFI_IP4_SERVICE_BINDING_PROTOCOL;
EFI_GUID Ip4Protocol = EFI_IP4_PROTOCOL;
EFI_GUID Udp4ServiceBindingProtocol = EFI_UDP4_SERVICE_BINDING_PROTOCOL;
EFI_GUID Udp4Protocol = EFI_UDP4_PROTOCOL;
EFI_GUID Tcp4ServiceBindingProtocol = EFI_TCP4_SERVICE_BINDING_PROTOCOL;
EFI_GUID Tcp4Protocol = EFI_TCP4_PROTOCOL;

//
// Pointer protocol GUIDs
//
EFI_GUID SimplePointerProtocol    = EFI_SIMPLE_POINTER_PROTOCOL_GUID;
EFI_GUID AbsolutePointerProtocol  = EFI_ABSOLUTE_POINTER_PROTOCOL_GUID;

//
// Debugger protocol GUIDs
//
EFI_GUID gEfiDebugImageInfoTableGuid           = EFI_DEBUG_IMAGE_INFO_TABLE_GUID;
EFI_GUID gEfiDebugSupportProtocolGuid          = EFI_DEBUG_SUPPORT_PROTOCOL_GUID;
