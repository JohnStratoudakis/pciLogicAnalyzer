// PCI WDM reference design driver for the Dragon board
// (c) fpga4fun.com KNJN LLC - 2004
//
// DragonPCI.c
// Based on "genport.c" from Microsoft Win2000 DDK

#include "DragonPCI.h"

NTSTATUS DriverEntry( IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER (RegistryPath);
    
    DriverObject->MajorFunction[IRP_MJ_CREATE]         = GpdDispatch;
    DriverObject->MajorFunction[IRP_MJ_CLOSE]          = GpdDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = GpdDispatch;
    DriverObject->DriverUnload                         = GpdUnload;
    DriverObject->MajorFunction[IRP_MJ_PNP]            = GpdDispatchPnp;
    DriverObject->MajorFunction[IRP_MJ_POWER]          = GpdDispatchPower;
    DriverObject->MajorFunction[IRP_MJ_SYSTEM_CONTROL] = GpdDispatchSystemControl;
    DriverObject->DriverExtension->AddDevice           = GpdAddDevice;

    return STATUS_SUCCESS;
}

NTSTATUS GpdAddDevice( IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PhysicalDeviceObject)
/*
Routine Description:
    The Plug & Play subsystem is handing us a brand new PDO, for which we
    (by means of INF registration) have been asked to provide a driver.

    We need to determine if we need to be in the driver stack for the device.
    Create a functional device object to attach to the stack
    Initialize that device object
    Return status success.

    Remember: we can NOT actually send ANY non pnp IRPS to the given driver
    stack, UNTIL we have received an IRP_MN_START_DEVICE.

Arguments:
    DeviceObject - pointer to a device object.
    PhysicalDeviceObject -  pointer to a device object created by the underlying bus driver.
*/
{
    NTSTATUS                status = STATUS_SUCCESS;
    PDEVICE_OBJECT          deviceObject = NULL;
    PLOCAL_DEVICE_INFO      deviceInfo; 
    UNICODE_STRING          ntDeviceName;
    UNICODE_STRING          win32DeviceName;

    RtlInitUnicodeString(&ntDeviceName, GPD_DEVICE_NAME);

    status = IoCreateDevice (DriverObject, sizeof (LOCAL_DEVICE_INFO), &ntDeviceName, FILE_DEVICE_UNKNOWN, 0, FALSE, &deviceObject);
    if (!NT_SUCCESS (status)) return status;

    RtlInitUnicodeString(&win32DeviceName, DOS_DEVICE_NAME);
    status = IoCreateSymbolicLink( &win32DeviceName, &ntDeviceName );
    if (!NT_SUCCESS(status)) { IoDeleteDevice(deviceObject); return status;}

    deviceInfo = (PLOCAL_DEVICE_INFO) deviceObject->DeviceExtension;
    deviceInfo->NextLowerDriver = IoAttachDeviceToDeviceStack(deviceObject, PhysicalDeviceObject);
    if(NULL == deviceInfo->NextLowerDriver) { IoDeleteSymbolicLink(&win32DeviceName); IoDeleteDevice(deviceObject); return STATUS_NO_SUCH_DEVICE;}

    deviceObject->Flags |=  DO_POWER_PAGABLE;
    deviceInfo->DeviceObject = deviceObject;
    deviceInfo->Removed = FALSE;
    deviceInfo->Started = FALSE;

    deviceObject->Flags &= ~DO_DEVICE_INITIALIZING;
    return STATUS_SUCCESS;
}

// Completion routine for plug & play irps that needs to be processed first by the lower drivers. 
NTSTATUS GpdCompletionRoutine( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp, IN PVOID Context)
{
    PKEVENT             event;
    event = (PKEVENT) Context;
    UNREFERENCED_PARAMETER(DeviceObject);

    if (Irp->PendingReturned) IoMarkIrpPending(Irp);
    KeSetEvent(event, 0, FALSE);
    return STATUS_MORE_PROCESSING_REQUIRED;    // Allows the caller to reuse the IRP
}

NTSTATUS GpdDispatchPnp( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
/*
Routine Description:
    The plug and play dispatch routines.
    Most of these the driver will completely ignore.
    In all cases it must pass the IRP to the next lower driver.

Arguments:
   DeviceObject - pointer to a device object.
   Irp - pointer to an I/O Request Packet.
*/
{
    PIO_STACK_LOCATION          irpStack;
    NTSTATUS                    status = STATUS_SUCCESS;
    KEVENT                      event;        
    UNICODE_STRING              win32DeviceName;
    PLOCAL_DEVICE_INFO          deviceInfo;

    deviceInfo = (PLOCAL_DEVICE_INFO) DeviceObject->DeviceExtension;
    irpStack = IoGetCurrentIrpStackLocation(Irp);

    switch (irpStack->MinorFunction) {
    case IRP_MN_START_DEVICE:
        // We cannot touch the device (send it any non-pnp irps) until a
        // start device has been passed down to the lower drivers.
        //
        IoCopyCurrentIrpStackLocationToNext(Irp);
        KeInitializeEvent(&event, NotificationEvent, FALSE );
        IoSetCompletionRoutine(Irp, (PIO_COMPLETION_ROUTINE) GpdCompletionRoutine, &event, TRUE, TRUE, TRUE);
                               
        status = IoCallDriver(deviceInfo->NextLowerDriver, Irp);
        if (STATUS_PENDING == status) KeWaitForSingleObject(&event, Executive, KernelMode, FALSE, NULL);

        if (NT_SUCCESS(status) && NT_SUCCESS(Irp->IoStatus.Status)) {
            status = GpdStartDevice(DeviceObject, Irp);
            if(NT_SUCCESS(status))
            {
                deviceInfo->Started = TRUE;
                deviceInfo->Removed = FALSE;
            }
        }

        Irp->IoStatus.Status = status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;

    case IRP_MN_QUERY_STOP_DEVICE:
        // Fail the query stop to prevent the system from taking away hardware 
        // resources. If you do support this you must have a queue to hold
        // incoming requests between stop and subsequent start with new set of
        // resources.
        //
        Irp->IoStatus.Status = status = STATUS_UNSUCCESSFUL;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        break;
        
    case IRP_MN_QUERY_REMOVE_DEVICE:
        // The device can be removed without disrupting the machine. 
        //
        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(deviceInfo->NextLowerDriver, Irp);
        break;

    case IRP_MN_SURPRISE_REMOVAL:
        // The device has been unexpectedly removed from the machine 
        // and is no longer available for I/O. Stop all access to the device.
        // Release any resources associated with the device, but leave the 
        // device object attached to the device stack until the PnP Manager 
        // sends a subsequent IRP_MN_REMOVE_DEVICE request. 
        // You should fail any outstanding I/O to the device. You will
        // not get a remove until all the handles open to the device
        // have been closed.
        //
        deviceInfo->Removed = TRUE;
        deviceInfo->Started = FALSE;
       
        if (deviceInfo->PortWasMapped)
        {
            MmUnmapIoSpace(deviceInfo->PortBase, deviceInfo->PortCount);
            deviceInfo->PortWasMapped = FALSE;
        }
        RtlInitUnicodeString(&win32DeviceName, DOS_DEVICE_NAME);
        IoDeleteSymbolicLink(&win32DeviceName);           
        
        IoSkipCurrentIrpStackLocation(Irp);
        Irp->IoStatus.Status = STATUS_SUCCESS;
        status = IoCallDriver(deviceInfo->NextLowerDriver, Irp);
        break;       
        
    case IRP_MN_REMOVE_DEVICE:
        // Relinquish all resources here.
        // Detach and delete the device object so that
        // your driver can be unloaded. You get remove
        // either after query_remove or surprise_remove.
        //
        if(!deviceInfo->Removed)
        {
            deviceInfo->Removed = TRUE;
            deviceInfo->Started = FALSE;

            if (deviceInfo->PortWasMapped)
            {
                MmUnmapIoSpace(deviceInfo->PortBase, deviceInfo->PortCount);
                deviceInfo->PortWasMapped = FALSE;
            }
            RtlInitUnicodeString(&win32DeviceName, DOS_DEVICE_NAME);
            IoDeleteSymbolicLink(&win32DeviceName);           
        }        

        Irp->IoStatus.Status = STATUS_SUCCESS;
        IoSkipCurrentIrpStackLocation(Irp);
        status = IoCallDriver(deviceInfo->NextLowerDriver, Irp);

        IoDetachDevice(deviceInfo->NextLowerDriver); 
        IoDeleteDevice(DeviceObject);
        return status;
                
    case IRP_MN_STOP_DEVICE:
        // Since you failed query stop, you will not get this request.
    case IRP_MN_CANCEL_REMOVE_DEVICE: 
        // No action required in this case. Just pass it down.
    case IRP_MN_CANCEL_STOP_DEVICE: 
        //No action required in this case.
        Irp->IoStatus.Status = STATUS_SUCCESS;
    default:
        IoSkipCurrentIrpStackLocation (Irp);
        status = IoCallDriver(deviceInfo->NextLowerDriver, Irp);
        break;
    }
    return status;
}

NTSTATUS GpdStartDevice( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
{
    NTSTATUS    status = STATUS_UNSUCCESSFUL;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resource;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR resourceTrans;
    PCM_PARTIAL_RESOURCE_LIST   partialResourceList;
    PCM_PARTIAL_RESOURCE_LIST   partialResourceListTranslated;
    PIO_STACK_LOCATION  stack;
    ULONG i;
    PLOCAL_DEVICE_INFO deviceInfo;

    deviceInfo = (PLOCAL_DEVICE_INFO) DeviceObject->DeviceExtension;
    stack = IoGetCurrentIrpStackLocation (Irp);

    if (deviceInfo->Removed) {
        // Some kind of surprise removal arrived. We will fail the IRP
        // The dispatch routine that called us will take care of completing the IRP.
        //
        return STATUS_DELETE_PENDING;
    }

    // Do whatever initialization needed when starting the device: 
    // gather information about it,  update the registry, etc.
    //
    if ((NULL == stack->Parameters.StartDevice.AllocatedResources) &&
        (NULL == stack->Parameters.StartDevice.AllocatedResourcesTranslated)) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    // Parameters.StartDevice.AllocatedResources points to a 
    // CM_RESOURCE_LIST describing the hardware resources that 
    // the PnP Manager assigned to the device. This list contains 
    // the resources in raw form. Use the raw resources to program 
    // the device.
    //
    partialResourceList = &stack->Parameters.StartDevice.AllocatedResources->List[0].PartialResourceList;
    resource = &partialResourceList->PartialDescriptors[0];
    
    // Parameters.StartDevice.AllocatedResourcesTranslated points 
    // to a CM_RESOURCE_LIST describing the hardware resources that 
    // the PnP Manager assigned to the device. This list contains 
    // the resources in translated form. Use the translated resources 
    // to connect the interrupt vector, map I/O space, and map memory.
    //
    partialResourceListTranslated = &stack->Parameters.StartDevice.AllocatedResourcesTranslated->List[0].PartialResourceList;
    resourceTrans = &partialResourceListTranslated->PartialDescriptors[0];

	deviceInfo->PortWasMapped = FALSE;
    deviceInfo->PortMemoryType = -1; 
    for (i=0; i<partialResourceList->Count; i++, resource++, resourceTrans++)
	{
        switch (resource->Type)
		{
        case CmResourceTypePort:

            switch (resourceTrans->Type)
			{
            case CmResourceTypePort:
                deviceInfo->PortWasMapped = FALSE;
                deviceInfo->PortBase = (PVOID)resourceTrans->u.Port.Start.LowPart;
                deviceInfo->PortCount = resourceTrans->u.Port.Length;
				deviceInfo->PortMemoryType = 1;
                status = STATUS_SUCCESS;
                break;

            case CmResourceTypeMemory:
                deviceInfo->PortBase = MmMapIoSpace (resourceTrans->u.Memory.Start, resourceTrans->u.Memory.Length, MmNonCached);
                deviceInfo->PortCount = resourceTrans->u.Memory.Length;
                deviceInfo->PortWasMapped = TRUE;
				deviceInfo->PortMemoryType = 0;
                status = STATUS_SUCCESS;
                break;
            }             
            break;

        case CmResourceTypeMemory:
            deviceInfo->PortBase = MmMapIoSpace (resourceTrans->u.Memory.Start, resourceTrans->u.Memory.Length, MmNonCached);
            deviceInfo->PortCount = resourceTrans->u.Memory.Length;
            deviceInfo->PortWasMapped = TRUE;
			deviceInfo->PortMemoryType = 0;
            status = STATUS_SUCCESS;
            break;
        }

//		if(	deviceInfo->PortMemoryType>=0 ) break;
    }

    return status;
}

NTSTATUS GpdDispatchPower( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
//    This routine is the dispatch routine for power irps.
//    Does nothing except forwarding the IRP to the next device in the stack.
{
    PLOCAL_DEVICE_INFO   deviceInfo;
    
    deviceInfo = (PLOCAL_DEVICE_INFO) DeviceObject->DeviceExtension;

    // If the device has been removed, the driver should not pass 
    // the IRP down to the next lower driver.
    //
    if (deviceInfo->Removed) {
        PoStartNextPowerIrp(Irp);
        Irp->IoStatus.Status =  STATUS_DELETE_PENDING;
        IoCompleteRequest(Irp, IO_NO_INCREMENT );
        return STATUS_DELETE_PENDING;
    }
    
    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(deviceInfo->NextLowerDriver, Irp);
}

NTSTATUS GpdDispatchSystemControl( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp)
//    This routine is the dispatch routine for WMI irps.
//    Does nothing except forwarding the IRP to the next device in the stack.
{
    PLOCAL_DEVICE_INFO   deviceInfo;

    deviceInfo = (PLOCAL_DEVICE_INFO) DeviceObject->DeviceExtension;
    IoSkipCurrentIrpStackLocation(Irp);
    return IoCallDriver(deviceInfo->NextLowerDriver, Irp);
}

VOID GpdUnload( IN PDRIVER_OBJECT DriverObject)
{
    // The device object(s) should be NULL now
    // (since we unload, all the devices objects associated with this
    // driver must have been deleted.
    //
//    ASSERT(DriverObject->DeviceObject == NULL);
    return;
}

NTSTATUS GpdIoctl( IN PLOCAL_DEVICE_INFO pLDI, IN PIRP pIrp, IN PIO_STACK_LOCATION pIrpStack)
{
	ULONG IoctlCode = pIrpStack->Parameters.DeviceIoControl.IoControlCode;
    PULONG pIOBuffer = pIrp->AssociatedIrp.SystemBuffer;
    ULONG InBufferSize  = pIrpStack->Parameters.DeviceIoControl.InputBufferLength;
    ULONG OutBufferSize = pIrpStack->Parameters.DeviceIoControl.OutputBufferLength;
    ULONG Addr;

    switch (IoctlCode)
    {
		case IOCTL_READ: 
			Addr = *pIOBuffer;
		    pIrp->IoStatus.Information = 4;
			if( InBufferSize!=sizeof(ULONG) || OutBufferSize!=sizeof(ULONG) || Addr>0xF ) return STATUS_INVALID_PARAMETER;

			Addr *= 4;
			switch(pLDI->PortMemoryType)
			{
			case 0:
				*(PULONG)pIOBuffer = READ_REGISTER_ULONG( (PULONG)((ULONG_PTR)pLDI->PortBase + Addr) );
				break;
			case 1:
				*(PULONG)pIOBuffer = READ_PORT_ULONG( (PULONG)((ULONG_PTR)pLDI->PortBase + Addr) );
				break;
			}
			break;

		case IOCTL_WRITE:
			Addr = *pIOBuffer++;
		    pIrp->IoStatus.Information = 0;
			if( InBufferSize!=2*sizeof(ULONG) || OutBufferSize!=0 || Addr>0xF ) return STATUS_INVALID_PARAMETER;

			Addr *= 4;
			switch(pLDI->PortMemoryType)
			{
			case 0:
				WRITE_REGISTER_ULONG( (PULONG)((ULONG_PTR)pLDI->PortBase + Addr), *pIOBuffer );
				break;
			case 1:
				WRITE_PORT_ULONG( (PULONG)((ULONG_PTR)pLDI->PortBase + Addr), *pIOBuffer );
				break;
			}
			break;

		default:      
			return STATUS_INVALID_PARAMETER;
    }

    return STATUS_SUCCESS;
}

NTSTATUS GpdDispatch( IN PDEVICE_OBJECT pDO, IN PIRP pIrp)
{
    PLOCAL_DEVICE_INFO pLDI;
    PIO_STACK_LOCATION pIrpStack;
    NTSTATUS Status;

    pIrp->IoStatus.Information = 0;
    pLDI = (PLOCAL_DEVICE_INFO)pDO->DeviceExtension;    // Get local info struct

	// We fail all the IRPs that arrive before the device is started.
    if (!pLDI->Started) {
        pIrp->IoStatus.Status = Status = STATUS_DEVICE_NOT_READY;
        IoCompleteRequest(pIrp, IO_NO_INCREMENT );
        return Status;
    }
    
    pIrpStack = IoGetCurrentIrpStackLocation(pIrp);
    switch (pIrpStack->MajorFunction)
    {
        case IRP_MJ_CREATE:
            Status = STATUS_SUCCESS;
            break;

        case IRP_MJ_CLOSE:
            Status = STATUS_SUCCESS;
            break;

        case IRP_MJ_DEVICE_CONTROL:
			Status = GpdIoctl( pLDI, pIrp, pIrpStack);
            break;

        default: 
            Status = STATUS_NOT_IMPLEMENTED;
            break;
    }

    pIrp->IoStatus.Status = Status;
    IoCompleteRequest(pIrp, IO_NO_INCREMENT );
    return Status;
}

