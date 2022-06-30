#include <arch/x86-64/apic/apic.h>

namespace APIC{ 
    //processors
    LocalProcessor** Processor;
    uint8_t ProcessorCount;

    LapicAddress** lapicAddress;

    //IOAPIC
    IOAPIC** IOapic;
    uint64_t IOAPICCount;

    //ISO
    InterruptSourceOverride** Iso;
    uint64_t IsoCount;

    void InitializeMADT(ACPI::MADTHeader* madt){
        ProcessorCount = 0;
        IsoCount = 0;
        uint64_t MaxAPICID = 1;

        uint64_t entries = (madt->Header.Length - sizeof(ACPI::MADTHeader));

        for(uint64_t i = 0; i < entries;){
            EntryRecord* entryRecord = (EntryRecord*)((uint64_t)madt + sizeof(ACPI::MADTHeader) + i);
            i += entryRecord->Length;

            switch(entryRecord->Type){
                case EntryTypeLocalProcessor: {
                    LocalProcessor* processor = (LocalProcessor*)entryRecord;
                    if(processor->APICID > MaxAPICID){
                        MaxAPICID = processor->APICID;
                    }  
                    ProcessorCount++;      
                    break;                    
                }
                case EntryTypeIOAPIC:{
                    IOAPICCount++;
                    break;
                }                    
                case EntryTypeInterruptSourceOverride:{
                    IsoCount++;
                    break;
                }                    
                case EntryTypeNonmaskableinterrupts:{
                    break;
                }                    
                case EntryTypeLocalAPICAddressOverride:{
                    break;  
                }                                      
            }
        }

        Processor = (LocalProcessor**)malloc(sizeof(LocalProcessor) * ProcessorCount);
        lapicAddress = (LapicAddress**)malloc(sizeof(LapicAddress) * MaxAPICID);

        IOapic = (IOAPIC**)malloc(sizeof(IOAPIC) * IOAPICCount);

        Iso = (InterruptSourceOverride**)malloc(sizeof(InterruptSourceOverride) * IsoCount);

        uint8_t ProcessorCountTmp = 0;
        uint64_t IsoCountTmp = 0;
        uint8_t IOAPICTmp = 0;

        for(uint64_t i = 0; i < entries;){            
            EntryRecord* entryRecord = (EntryRecord*)((uint64_t)madt + sizeof(ACPI::MADTHeader) + i);
            i += entryRecord->Length;

            switch(entryRecord->Type){
                case EntryTypeLocalProcessor: {
                    LocalProcessor* processor = (LocalProcessor*)entryRecord;
                    Processor[ProcessorCountTmp] = processor;  
                    lapicAddress[processor->APICID] = (LapicAddress*)malloc(sizeof(LapicAddress));
                    ProcessorCountTmp++;
                    break;
                }
                case EntryTypeIOAPIC:{
                    IOAPIC* ioApic = (IOAPIC*)entryRecord;
                    IOapic[IOAPICTmp] = ioApic;
                    IOAPICTmp++;
                    break;
                }                    
                case EntryTypeInterruptSourceOverride:{
                    InterruptSourceOverride* iso = (InterruptSourceOverride*)entryRecord;
                    Iso[IsoCountTmp] = iso;
                    IsoCountTmp++;
                    break;
                }                    
                case EntryTypeNonmaskableinterrupts:{
                    NonMaskableinterrupts* nmi = (NonMaskableinterrupts*)entryRecord;
                    break;
                }                    
                case EntryTypeLocalAPICAddressOverride:{
                    LocalAPICAddressOverride* local_apic_override = (LocalAPICAddressOverride*)entryRecord;
                    break;  
                }                                      
            }
        }
        IoAPICInit(0);
    }  

    void IoAPICInit(uint8_t IOApicID){
        // Disable PIC
        IoWrite8(0xa1, 0xff);
        IoWrite8(0x21, 0xff);

        // Configure first IOAPIC
        IOAPIC* ioapic = IOapic[IOApicID];
        uint64_t IOapicAddressVirtual = vmm_Map((uintptr_t)(uint64_t)ioapic->APICAddress);
        uint8_t MaxInterrupts = ((ioapicReadRegister((uintptr_t)IOapicAddressVirtual, IOAPICVersion) >> 16) & 0xff) + 1;
        ioapic->MaxInterrupts = MaxInterrupts;

        // Set up the entries
        uint32_t base = ioapic->GlobalSystemInterruptBase;
        for (size_t i = 0; i < MaxInterrupts; i++){
                uint8_t IRQNumber = i + IRQ_START;
                IoApicSetRedirectionEntry((uintptr_t)IOapicAddressVirtual, i - base, (IOAPICRedirectionEntry){
                    .vector = IRQNumber,
                    .delivery_mode = IOAPICRedirectionEntryDeliveryModeFixed,
                    .destination_mode = IOAPICRedirectionEntryDestinationModePhysicall,
                    .delivery_status = IOAPICRedirectionEntryDeliveryStatusIddle,
                    .pin_polarity = IOAPICRedirectionEntryPinPolarityActiveHigh,
                    .remote_irr = IOAPICRedirectionEntryRemoteIRRNone,
                    .trigger_mode = IOAPICRedirectionEntryTriggerModeEdge,
                    .mask = IOAPICRedirectionEntryMaskDisable,
                    .destination = NULL,
                });
        }

        for(size_t i = 0; i < IsoCount; i++) {
            InterruptSourceOverride* iso = Iso[i];
            uint8_t IRQNumber = iso->IRQSource + IRQ_START;
            IoApicSetRedirectionEntry((uintptr_t)IOapicAddressVirtual , iso->IRQSource, (IOAPICRedirectionEntry){
                .vector = IRQNumber,
                .delivery_mode = IOAPICRedirectionEntryDeliveryModeFixed,
                .destination_mode = IOAPICRedirectionEntryDestinationModePhysicall,
                .delivery_status = IOAPICRedirectionEntryDeliveryStatusIddle,
                .pin_polarity = (iso->Flags & 0x03) == 0x03 ? IOAPICRedirectionEntryPinPolarityActiveLow : IOAPICRedirectionEntryPinPolarityActiveHigh,
                .remote_irr = IOAPICRedirectionEntryRemoteIRRNone,
                .trigger_mode = (iso->Flags & 0x0c) == 0x0c ? IOAPICRedirectionEntryTriggerModeLevel : IOAPICRedirectionEntryTriggerModeEdge,
                .mask = IOAPICRedirectionEntryMaskDisable,
                .destination = NULL,
            });
        } 

    } 

    void IoChangeIrqState(uint8_t irq, uint8_t IOApicID, bool IsEnable){
        IOAPIC* ioapic = IOapic[IOApicID];
        uint64_t IOapicAddressVirtual = vmm_Map((uintptr_t)(uint64_t)ioapic->APICAddress);
        uint32_t base = ioapic->GlobalSystemInterruptBase;
        size_t index = irq - base;
        
        volatile uint32_t low = 0;

        low = ioapicReadRegister((uintptr_t)IOapicAddressVirtual, IOAPICRedirectionTable + 2 * index);
        
        if(!IsEnable){
            low |= 1 << IOAPICRedirectionBitsLowMask;
        }else{
            low &= ~(1 << IOAPICRedirectionBitsLowMask);
        }

        ioapicWriteRegister((uintptr_t)IOapicAddressVirtual, IOAPICRedirectionTable + 2 * index, low);
    }

    void LoadCores(){
        uintptr_t TrampolineVirtualAddress = (uintptr_t)vmm_GetVirtualAddress(0x1000);

        memcpy((uintptr_t)TrampolineVirtualAddress, (uintptr_t)&Trampoline, PAGE_SIZE);

        trampolineData* Data = (trampolineData*)(((uint64_t)&DataTrampoline - (uint64_t)&Trampoline) + (uint64_t)TrampolineVirtualAddress);

        
        //temp trampoline map
        vmm_Map((uintptr_t)TRAMPOLINE_ADDRESS, (uintptr_t)TRAMPOLINE_ADDRESS);

        for(int i = 0; i < ProcessorCount; i++){ 
            if(Processor[i]->APICID == Processor[CPU::GetAPICID()]->APICID) continue; 

            Data->Paging = (uint64_t)vmm_PageTable;
            Data->MainEntry = (uint64_t)&TrampolineMain; 
            Data->Stack = (uint64_t)malloc(KERNEL_STACK_SIZE) + KERNEL_STACK_SIZE;
            memset((uintptr_t)(Data->Stack - KERNEL_STACK_SIZE), 0xff, KERNEL_STACK_SIZE);

            lapicSendInitIPI(Processor[i]->APICID);

            DataTrampoline.Status = 0;
            // send STARTUP IPI twice 
            lapicSendStartupIPI(Processor[i]->APICID, (uintptr_t)TRAMPOLINE_ADDRESS);
            
            Warning("Wait processor %u", i);

            while (DataTrampoline.Status != 0xef){
                __asm__ __volatile__ ("pause" : : : "memory");
            } 
            Successful("Processor %u respond with success", i);
        }
        DataTrampoline.Status = 0xff;
        vmm_Unmap((uintptr_t)TRAMPOLINE_ADDRESS);

    }  

    uintptr_t GetLAPICAddress(){
        return lapicAddress[CPU::GetAPICID()]->VirtualAddress;
    }

    void EnableAPIC(uint8_t CoreID){
        lapicAddress[CoreID]->PhysicalAddress = (uintptr_t)(msr::rdmsr(0x1b) & 0xfffff000);
        lapicAddress[CoreID]->VirtualAddress = (uintptr_t)vmm_Map(lapicAddress[CoreID]->PhysicalAddress); 
        msr::wrmsr(0x1b, ((uint64_t)lapicAddress[CoreID]->PhysicalAddress | LOCAL_APIC_ENABLE) & ~((1 << 10)));
        localAPICWriteRegister(LocalAPICRegisterOffsetSpuriousIntVector, localAPICReadRegister(LocalAPICRegisterOffsetSpuriousIntVector) | (LOCAL_APIC_SPURIOUS_ALL | LOCAL_APIC_SPURIOUS_ENABLE_APIC));
    }



    static uint64_t mutexSLT;

    void StartLapicTimer(){
        Atomic::atomicAcquire(&mutexSLT, 0);

        // Setup Local APIC timer
        localAPICWriteRegister(LocalAPICRegisterOffsetDivide, 4);        
        localAPICWriteRegister(LocalAPICRegisterOffsetInitialCount, 0xffffffff);

        HPET::HPETSleep(10);
        
        uint32_t Tick10ms = 0xffffffff - localAPICReadRegister(LocalAPICRegisterOffsetCurentCount);

        LocalAPICInterruptRegister TimerRegisters;

        /* Don't forget to define all the struct because it can be corrupt by the stack */
        TimerRegisters.vector = IPI_Schedule;
        TimerRegisters.messageType = LocalAPICInterruptRegisterMessageTypeFixed;
        TimerRegisters.deliveryStatus = LocalAPICInterruptRegisterMessageTypeIddle;
        TimerRegisters.remoteIrr = LocalAPICInterruptRegisterRemoteIRRCompleted;
        TimerRegisters.triggerMode = LocalAPICInterruptRegisterTriggerModeEdge;
        TimerRegisters.mask = LocalAPICInterruptRegisterMaskEnable;
        TimerRegisters.timerMode = LocalAPICInterruptTimerModePeriodic;
        
        uint32_t timer = localAPICReadRegister(LocalAPICRegisterOffsetLVTTimer);
        localAPICWriteRegister(LocalAPICRegisterOffsetLVTTimer, CreateRegisterValueInterrupts(TimerRegisters) | (timer & 0xfffcef00));    
        localAPICWriteRegister(LocalAPICRegisterOffsetInitialCount, (Tick10ms / 10)); 
        Atomic::atomicUnlock(&mutexSLT, 0);
    }

    void localAPICSetTimerCount(uint32_t value){
        localAPICWriteRegister(LocalAPICRegisterOffsetInitialCount, value);
    }

    uint32_t localAPICGetTimerCount(){
        return localAPICReadRegister(LocalAPICRegisterOffsetCurentCount);
    }

    void lapicSendInitIPI(uint8_t CoreID){
        LocalAPICIipi registerInterrupt;
        registerInterrupt.vector = 0;
        registerInterrupt.deliveryMode = LocalAPICDeliveryModeINIT;
        registerInterrupt.destinationMode = LocalAPICDestinationModePhysicalDestination;
        registerInterrupt.destinationType = LocalAPICDestinationTypeBase;
        uint32_t commandLow = CreateLocalAPICIipiRegister(registerInterrupt);
        uint32_t commandHigh = CoreID << 24;
        SetCommandIPI(commandLow, commandHigh);
    }

    void lapicSendStartupIPI(uint8_t CoreID, uintptr_t entry){
        LocalAPICIipi registerInterrupt;
        registerInterrupt.vector = (uint8_t)(((uint64_t)entry / PAGE_SIZE) & 0xff);
        registerInterrupt.deliveryMode = LocalAPICDeliveryModeStartUp;
        registerInterrupt.destinationMode = LocalAPICDestinationModePhysicalDestination;
        registerInterrupt.destinationType = LocalAPICDestinationTypeBase;
        uint32_t commandLow = CreateLocalAPICIipiRegister(registerInterrupt);
        uint32_t commandHigh = CoreID << 24;
        SetCommandIPI(commandLow, commandHigh);
    }

    void localApicEOI(uint8_t CoreID){        
        localAPICWriteRegister(lapicAddress[CoreID]->VirtualAddress, LocalAPICRegisterOffsetEOI, 0);
    }

    void localApicEnableSpuriousInterrupts(){
        localAPICWriteRegister(LocalAPICRegisterOffsetSpuriousIntVector, localAPICReadRegister(LocalAPICRegisterOffsetSpuriousIntVector) | 0x100);
    }

    /* APIC */

    uint32_t localAPICReadRegister(size_t offset){
        uintptr_t lapicAddress = GetLAPICAddress();
	    return *((volatile uint32_t*)((uintptr_t)((uint64_t)lapicAddress + offset)));
    }

    uint32_t localAPICReadRegister(uintptr_t lapicAddress, size_t offset){
	    return *((volatile uint32_t*)((uintptr_t)((uint64_t)lapicAddress + offset)));
    }

    uint32_t ioapicReadRegister(uintptr_t apicPtr , uint8_t offset){
        *(volatile uint32_t*)(apicPtr) = offset;
        return *(volatile uint32_t*)((uint64_t)apicPtr + 0x10);
    }

    void ioapicWriteRegister(uintptr_t apicPtr , uint8_t offset, uint32_t value){
        *(volatile uint32_t*)(apicPtr) = offset;
        *(volatile uint32_t*)((uint64_t)apicPtr + 0x10) = value;
    }
    
    void localAPICWriteRegister(size_t offset, uint32_t value){
        uintptr_t lapicAddress = GetLAPICAddress();
        *((volatile uint32_t*)((uintptr_t)((uint64_t)lapicAddress + offset))) = value;
    }

    void localAPICWriteRegister(uintptr_t lapicAddress, size_t offset, uint32_t value){
        *((volatile uint32_t*)((uintptr_t)((uint64_t)lapicAddress + offset))) = value;
    }

    uint32_t CreateRegisterValueInterrupts(LocalAPICInterruptRegister reg){
        return (
            (reg.vector << LocalAPICInterruptVector) |
            (reg.messageType << LocalAPICInterruptMessageType) |
            (reg.deliveryStatus << LocalAPICInterruptDeliveryStatus) |
            (reg.triggerMode << LocalAPICInterruptTrigerMode) |
            (reg.mask << LocalAPICInterruptMask) |
            (reg.timerMode << LocalAPICInterruptTimerMode)
        );
    }

    void IoApicSetRedirectionEntry(uintptr_t apicPtr, size_t index, IOAPICRedirectionEntry entry){
        volatile uint32_t low = (
            (entry.vector << IOAPICRedirectionBitsLowVector) |
            (entry.delivery_mode << IOAPICRedirectionBitsLowDeliveryMode) |
            (entry.destination_mode << IOAPICRedirectionBitsLowDestinationMode) |
            (entry.delivery_status << IOIOAPICRedirectionBitsLowDeliveryStatus) |
            (entry.pin_polarity << IOAPICRedirectionBitsLowPonPolarity) |
            (entry.remote_irr << IOAPICRedirectionBitsLowRemoteIrr) |
            (entry.trigger_mode << IOAPICRedirectionBitsLowTriggerMode) |
            (entry.mask << IOAPICRedirectionBitsLowMask)
        );
        volatile uint32_t high = (
            (entry.destination << IOAPICRedirectionBitsHighDestination)
        );
        
        ioapicWriteRegister(apicPtr, IOAPICRedirectionTable + 2 * index, low);
        ioapicWriteRegister(apicPtr, IOAPICRedirectionTable + 2 * index + 1, high);
    }

    void SetCommandIPI(uint32_t commandLow, uint32_t commandHigh){
        uintptr_t lapicAddress = GetLAPICAddress();
        localAPICWriteRegister(LocalAPICRegisterOffsetErrorStatus, 0);
        localAPICWriteRegister(lapicAddress, LocalAPICRegisterOffsetInterruptCommandHigh, commandHigh);
        localAPICWriteRegister(lapicAddress, LocalAPICRegisterOffsetInterruptCommandLow, commandLow);
        do { __asm__ __volatile__ ("pause" : : : "memory"); }while(localAPICReadRegister(LocalAPICRegisterOffsetInterruptCommandLow) & (1 << 12));
    }

    void GenerateInterruption(uint64_t CoreID, uint8_t Vector){
        LocalAPICIipi registerInterrupt;
        registerInterrupt.vector = Vector;
        registerInterrupt.deliveryMode = LocalAPICDeliveryModeFixed;
        registerInterrupt.destinationMode = LocalAPICDestinationModePhysicalDestination;
        registerInterrupt.destinationType = LocalAPICDestinationTypeBase;
        uint32_t commandLow = CreateLocalAPICIipiRegister(registerInterrupt);
        uint32_t commandHigh = CoreID << 24;
        
        SetCommandIPI(commandLow, commandHigh);
    }

    void GenerateInterruptionNMI(uint64_t CoreID){
        LocalAPICIipi registerInterrupt;
        registerInterrupt.vector = 0x0;
        registerInterrupt.deliveryMode = LocalAPICDeliveryModeNMI;
        registerInterrupt.destinationMode = LocalAPICDestinationModePhysicalDestination;
        registerInterrupt.destinationType = LocalAPICDestinationTypeBase;
        uint32_t commandLow = CreateLocalAPICIipiRegister(registerInterrupt);
        uint32_t commandHigh = CoreID << 24;
        
        SetCommandIPI(commandLow, commandHigh);
    }

    uint32_t CreateLocalAPICIipiRegister(LocalAPICIipi reg){
        return (
            (reg.vector << LocalAPICInterruptipiVector) |
            (reg.deliveryMode << LocalAPICInterruptipiMessageType) |
            (reg.destinationMode << LocalAPICInterruptipiDestinationMode) |
            (reg.destinationType << LocalAPICInterruptipiDestinationType)
        );
    }
}