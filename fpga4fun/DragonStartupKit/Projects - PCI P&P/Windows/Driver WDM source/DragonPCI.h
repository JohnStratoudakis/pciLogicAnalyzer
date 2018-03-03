
//
#define DEVICE_NAME L"DragonPCI"

//
//#include <ntddk.h>
#include <wdm.h>
#include "ioctl.h"

#if !defined(__DRAGONPCI_H__)
#define __DRAGONPCI_H__

#define GPD_DEVICE_NAME L"\\Device\\" DEVICE_NAME
#define DOS_DEVICE_NAME L"\\DosDevices\\" DEVICE_NAME

// driver local data structure specific to each device object
typedef struct _LOCAL_DEVICE_INFO {
    PVOID               PortBase;
    ULONG               PortCount;
    ULONG               PortMemoryType;
    BOOLEAN             PortWasMapped;
    BOOLEAN             Filler[1]; 		//bug fix

    PDEVICE_OBJECT      DeviceObject;
    PDEVICE_OBJECT      NextLowerDriver;
    BOOLEAN             Started;
    BOOLEAN             Removed;
}
LOCAL_DEVICE_INFO, *PLOCAL_DEVICE_INFO;

//////////////////////////////////////////////////////////////////
NTSTATUS    
DriverEntry(       
	IN  PDRIVER_OBJECT DriverObject,
	IN  PUNICODE_STRING RegistryPath 
	);

NTSTATUS    
GpdDispatch(       
    IN  PDEVICE_OBJECT pDO,
    IN  PIRP pIrp                    
    );

NTSTATUS    
GpdIoctl(  
    IN  PLOCAL_DEVICE_INFO pLDI,
    IN  PIRP pIrp,
    IN  PIO_STACK_LOCATION IrpStack
    );

VOID        
GpdUnload(         
    IN  PDRIVER_OBJECT DriverObject 
    );


NTSTATUS
GpdAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject
    );


NTSTATUS
GpdDispatchPnp (
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp
    );

NTSTATUS
GpdStartDevice (
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP             Irp
    );

NTSTATUS
GpdDispatchPower(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );
NTSTATUS 
GpdDispatchSystemControl(
    IN PDEVICE_OBJECT    DeviceObject,
    IN PIRP              Irp
    );

#endif
