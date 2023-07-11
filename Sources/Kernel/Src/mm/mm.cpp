#include <mm/mm.h>
#include <abi-bits/errno.h>
#include <abi-bits/vm-flags.h>

static KResult RemoveRegion(MemoryHandler_t* Handler, MemoryRegion_t* Region){
    if(Region->Last){
        Region->Last->Next = Region->Next;
    }
    if(Region->Next){
        Region->Next->Last = Region->Last;
    }
    Handler->RegionCount--;
    return KSUCCESS;
}

static MemoryRegion_t* SplitRegion(MemoryHandler_t* Handler, MemoryRegion_t* Region, size_t Size){
    if(Region->Size == Size){
        return Region;
    }else{
        if(Region->Size < Size){
            return Region;
        }

        MemoryRegion_t* NewRegion = (MemoryRegion_t*)kmalloc(sizeof(MemoryRegion_t));
        NewRegion->Base = (void*)((uintptr_t)Region->Base + Size);
        NewRegion->Size = Region->Size - Size;
        NewRegion->BlockCount = NewRegion->Size / BLOCK_SIZE;
        NewRegion->Flags = Region->Flags;
        NewRegion->Prot = Region->Prot;
        NewRegion->IsFree = Region->IsFree;
        NewRegion->IsMap = Region->IsMap;
        NewRegion->Last = Region;
        NewRegion->Next = Region->Next;


        Region->Next = NewRegion;
        Region->Size = Size;
        Region->BlockCount = Region->Size / BLOCK_SIZE;

        Handler->RegionCount++;

        return NewRegion;
    }
}

static MemoryRegion_t* MergeRegion(MemoryHandler_t* Handler, MemoryRegion_t* Region){
    if(Region->Next){
        MemoryRegion_t* From = Region->Next;
        Region->Size += From->Size;
        Region->BlockCount += From->BlockCount;
        RemoveRegion(Handler, From);
        return Region;
    }else{
        return NULL;
    }
}

MemoryHandler_t* MMCreateHandler(pagetable_t Paging, void* Base, size_t Size){
    MemoryHandler_t* Handler = (MemoryHandler_t*)kmalloc(sizeof(MemoryHandler_t));
    Handler->Base = Base;
    Handler->Size = Size;
    Handler->Paging = Paging;
    Handler->RegionCount = 0;

    MemoryRegion_t* Region = (MemoryRegion_t*)kmalloc(sizeof(MemoryRegion_t));
    Region->Base = Base;
    Region->Size = Size;
    Region->BlockCount = Size / BLOCK_SIZE;
    Region->Flags = 0;
    Region->Prot = PROT_NONE;
    Region->IsFree = true;
    Region->IsMap = false;
    Region->Last = NULL;
    Region->Next = NULL;

    Handler->FirstRegion = Region;
    Handler->LastFreeRegion = Region;

    Handler->RegionCount++;

    AtomicClearLock(&Handler->Lock);
    return Handler;
}

MemoryHandler_t* MMCloneHandler(pagetable_t Paging, MemoryHandler_t* Source){
    MemoryHandler_t* Destination = (MemoryHandler_t*)kmalloc(sizeof(MemoryHandler_t));
    Destination->Base = Source->Base;
    Destination->Size = Source->Size;
    Destination->Paging = Paging;
    Destination->RegionCount = Source->RegionCount;

    MemoryRegion_t* LastRegion = NULL;
    MemoryRegion_t* Region = Source->FirstRegion;
    for(uint64_t i = 0; i < Source->RegionCount; i++){
        MemoryRegion_t* RegionCopy = (MemoryRegion_t*)kmalloc(sizeof(MemoryRegion_t));

        memcpy(RegionCopy, Region, sizeof(MemoryRegion_t));

        if(LastRegion){
            RegionCopy->Last = LastRegion;
            LastRegion->Next = RegionCopy;
        }

        if(Region == Source->FirstRegion){
            Destination->FirstRegion = RegionCopy;
        }

        if(Region == Source->LastFreeRegion){
            Destination->LastFreeRegion = RegionCopy;
        }

        LastRegion = RegionCopy;

        Region = Region->Next;
    } 

    AtomicClearLock(&Destination->Lock);
    return Destination;    
}

KResult FreeHandler(MemoryHandler_t* Handler){
    MemoryRegion_t* Region = Handler->FirstRegion;
    for(uint64_t i = 0; i < Handler->RegionCount; i++){
        MemoryRegion_t* Next = Region->Next;
        kfree(Region);
        Region = Next;
    }
    kfree(Handler);
    return KSUCCESS;
}


MemoryRegion_t* MMAllocateRegionVM(MemoryHandler_t* Handler, void* Base, size_t Size, int Flags, int Prot, int* Errno){
    assert(!((uintptr_t)Base % PAGE_SIZE));
    assert(!(Size % PAGE_SIZE));

    AtomicAcquire(&Handler->Lock);

    MemoryRegion_t* Region = NULL;

    if(Flags & MAP_FIXED){
        MemoryRegion_t* Tmp = MMGetRegion(Handler, Base);
        if(!Tmp){
            AtomicRelease(&Handler->Lock);
            *Errno = EINVAL;
            return NULL;
        }
        if(Tmp->Base != Base){
            size_t SizeUnusable = (uintptr_t)Base - (uintptr_t)Tmp->Base;
            MemoryRegion_t* UsableRegion = SplitRegion(Handler, Tmp, SizeUnusable);
            while(UsableRegion->Size < Size){
                if(!MergeRegion(Handler, UsableRegion)){
                    *Errno = EINVAL;
                    return NULL;
                }
            }
            SplitRegion(Handler, UsableRegion, Size);
            Region = UsableRegion;
        }else{
            while(Tmp->Size < Size){
                if(!MergeRegion(Handler, Tmp)){
                    *Errno = EINVAL;
                    return NULL;
                }
            }
            SplitRegion(Handler, Tmp, Size);
            Region = Tmp;
        }
    }else{
        MemoryRegion_t* RegionToSplit = NULL; 
        MemoryRegion_t* Tmp = Handler->LastFreeRegion;
        for(uint64_t i = 0; i < Handler->RegionCount; i++){
            if(Tmp->IsFree){
                if(Tmp->Size >= Size){
                    RegionToSplit = Tmp;
                    break;
                }
            }
            Tmp = Tmp->Next;
        }
        if(!RegionToSplit){
            AtomicRelease(&Handler->Lock);
            *Errno = EINVAL;
            return NULL;
        }
        SplitRegion(Handler, RegionToSplit, Size);
        Region = RegionToSplit;
    }

    Region->IsFree = false;
    Region->Prot = Prot;
    Region->Flags = Flags;

    AtomicRelease(&Handler->Lock);

    return Region;
}

KResult MMFree(MemoryHandler_t* Handler, void* Base, size_t Size){
    // TODO

    return KSUCCESS;
}

MemoryRegion_t* MMGetRegion(MemoryHandler_t* Handler, void* Base){
    MemoryRegion_t* Region = Handler->FirstRegion;
    for(uint64_t i = 0; i < Handler->RegionCount; i++){
        if((uintptr_t)Region->Base <= (uintptr_t)Base && (uintptr_t)Region->Base + Region->Size > (uintptr_t)Base){
            return Region;
        }
        Region = Region->Next;
    }
    return NULL;
}


KResult MMAllocateMemoryBlock(MemoryHandler_t* Handler, MemoryRegion_t* Region){
    AtomicAcquire(&Handler->Lock);

    if(Region->IsFree){
        AtomicRelease(&Handler->Lock);
        return KFAIL;
    }


    if(Region->IsMap){
        AtomicRelease(&Handler->Lock);
        return KFAIL;
    }

    uintptr_t VirtualAddress = (uintptr_t)Region->Base;
    for(uint64_t i = 0; i < Region->BlockCount; i++){
        vmm_Map(Handler->Paging, (void*)VirtualAddress, Pmm_RequestPage(), Region->Prot & PROT_READ, Region->Prot & PROT_WRITE, true);
        VirtualAddress += BLOCK_SIZE;
    }

    Region->IsMap = true;

    AtomicRelease(&Handler->Lock);

    return KSUCCESS;
}

KResult MMAllocateMemoryBlockMaster(MemoryHandler_t* Handler, MemoryRegion_t* Region){
    AtomicAcquire(&Handler->Lock);

    if(Region->IsFree){
        AtomicRelease(&Handler->Lock);
        return KFAIL;
    }


    if(Region->IsMap){
        AtomicRelease(&Handler->Lock);
        return KFAIL;
    }

    uintptr_t VirtualAddress = (uintptr_t)Region->Base;
    for(uint64_t i = 0; i < Region->BlockCount; i++){
        vmm_Map(Handler->Paging, (void*)VirtualAddress, Pmm_RequestPage(), Region->Prot & PROT_READ, Region->Prot & PROT_WRITE, false);
        vmm_SetFlags(Handler->Paging, (void*)VirtualAddress, vmm_flag::vmm_Master, true); //set master state
        VirtualAddress += BLOCK_SIZE;
    }

    Region->IsMap = true;

    AtomicRelease(&Handler->Lock);

    return KSUCCESS;
}

KResult MMAllocateMemoryContigous(MemoryHandler_t* Handler, MemoryRegion_t* Region){
    AtomicAcquire(&Handler->Lock);

    if(Region->IsFree){
        AtomicRelease(&Handler->Lock);
        return KFAIL;
    }


    if(Region->IsMap){
        AtomicRelease(&Handler->Lock);
        return KFAIL;
    }


    uintptr_t VirtualAddress = (uintptr_t)Region->Base;
    uintptr_t PhysicalAddress = (uintptr_t)Pmm_RequestPages(Region->BlockCount);
    for(uint64_t i = 0; i < Region->BlockCount; i++){
        vmm_Map(Handler->Paging, (void*)VirtualAddress, (void*)PhysicalAddress, Region->Prot & PROT_READ, Region->Prot & PROT_WRITE, true);
        VirtualAddress += BLOCK_SIZE;
        PhysicalAddress += BLOCK_SIZE;
    }

    Region->IsMap = true;

    AtomicRelease(&Handler->Lock);

    return KSUCCESS;
}

KResult MMMapPhysical(MemoryHandler_t* Handler, MemoryRegion_t* Region, void* Base){
    AtomicAcquire(&Handler->Lock);

    if(Region->IsFree){
        AtomicRelease(&Handler->Lock);
        return KFAIL;
    }

    if(Region->IsMap){
        AtomicRelease(&Handler->Lock);
        return KFAIL;
    }

    uintptr_t VirtualAddress = (uintptr_t)Region->Base;
    uintptr_t PhysicalAddress = (uintptr_t)Base;
    for(uint64_t i = 0; i < Region->BlockCount; i++){
        vmm_Map(Handler->Paging, (void*)VirtualAddress, (void*)PhysicalAddress, Region->Prot & PROT_READ, Region->Prot & PROT_WRITE, true);
        VirtualAddress += BLOCK_SIZE;
        PhysicalAddress += BLOCK_SIZE;
    }

    Region->IsMap = true;

    AtomicRelease(&Handler->Lock);

    return KSUCCESS;
}

KResult MMUnmap(MemoryHandler_t* Handler, MemoryRegion_t* Region){
    AtomicAcquire(&Handler->Lock);

    if(Region->IsFree){
        AtomicRelease(&Handler->Lock);
        return KFAIL;
    }

    if(Region->IsMap){
        AtomicRelease(&Handler->Lock);
        return KFAIL;
    }

    uintptr_t VirtualAddress = (uintptr_t)Region->Base;
    for(uint64_t i = 0; i < Region->BlockCount; i++){
        if(vmm_GetFlags(Handler->Paging, (void*)VirtualAddress, vmm_flag::vmm_Master)){
            if(vmm_GetFlags(Handler->Paging, (void*)VirtualAddress, vmm_flag::vmm_IsPureMemory)){
                Pmm_FreePage(vmm_GetPhysical(Handler->Paging, (void*)VirtualAddress));
            }
            vmm_Unmap(Handler->Paging, (void*)VirtualAddress);
        }
    }

    Region->IsMap = false;

    AtomicRelease(&Handler->Lock);

    return KSUCCESS;
}

KResult MMProtect(MemoryHandler_t* Handler, void* Base, size_t Size, int Prot){
    // TODO
    
    return KSUCCESS;
}


/* _____________________________Share Memory_____________________________ */
//vmm_flag::vmm_Custom1 master share
//vmm_flag::vmm_Slave slave share

uint64_t MMCreateMemoryField(kthread_t* self, kprocess_t* process, size64_t size, uint64_t* virtualAddressPointer, uint64_t* keyPointer, enum MemoryFieldType type){
    if(CheckAddress(virtualAddressPointer, sizeof(uint64_t)) != KSUCCESS) return KFAIL;
    if(CheckAddress(keyPointer, sizeof(uint64_t)) != KSUCCESS) return KFAIL;
    void* virtualAddress = (void*)*virtualAddressPointer;
    uint64_t offset = NULL;
    pagetable_t pageTable = process->SharedPaging;
    uint64_t realSize = size;
    uint64_t numberOfPage = DivideRoundUp(realSize, PAGE_SIZE);


    switch(type){
        case MemoryFieldTypeShareSpaceRW:{
            offset = ((uint64_t)virtualAddress) % PAGE_SIZE;
            if((uint64_t)virtualAddress % PAGE_SIZE > 0){
                virtualAddress = (void*)((uint64_t)virtualAddress - (uint64_t)virtualAddress % PAGE_SIZE);
            }
            realSize += offset;
            if(realSize % PAGE_SIZE){
                realSize -= realSize % PAGE_SIZE;
                realSize += PAGE_SIZE;
            }

            int Errno = 0;
            MemoryRegion_t* Region = MMAllocateRegionVM(process->MemoryManager, (void*)virtualAddress, realSize, MAP_FIXED | MAP_SHARED, PROT_READ | PROT_WRITE | PROT_EXEC, &Errno);


            if(Region){
                MMAllocateMemoryBlockMaster(process->MemoryManager, Region);
            }else if(Errno != EEXIST){
                return KFAIL;
            }

            break;
        }
        case MemoryFieldTypeShareSpaceRO:{
            offset = ((uint64_t)virtualAddress) % PAGE_SIZE;
            if((uint64_t)virtualAddress % PAGE_SIZE > 0){
                virtualAddress = (void*)((uint64_t)virtualAddress - (uint64_t)virtualAddress % PAGE_SIZE);
            }
            realSize += offset;
            if(realSize % PAGE_SIZE){
                realSize -= realSize % PAGE_SIZE;
                realSize += PAGE_SIZE;
            }

            int Errno = 0;
            MemoryRegion_t* Region = MMAllocateRegionVM(process->MemoryManager, (void*)virtualAddress, realSize, MAP_FIXED | MAP_SHARED, PROT_READ | PROT_WRITE | PROT_EXEC, &Errno);

            if(Region){
                MMAllocateMemoryBlockMaster(process->MemoryManager, Region);
            }else if(Errno != EEXIST){
                return KFAIL;
            }

            break;
        }
        case MemoryFieldTypeSendSpaceRO:{
            pagetable_t lastPageTable = vmm_GetPageTable();
            if(CheckAddress(virtualAddress, realSize, pageTable) != KSUCCESS) return KFAIL;
            if(lastPageTable != pageTable) vmm_Swap(self, pageTable);
            if((uint64_t)virtualAddress % PAGE_SIZE){
                size64_t nonAlignedSize = PAGE_SIZE - ((uint64_t)virtualAddress % PAGE_SIZE);
                if(realSize > nonAlignedSize){
                    numberOfPage++;
                }
            }
            if(lastPageTable != pageTable) vmm_Swap(self, lastPageTable);
            break;
        }
    }

    MemoryShareInfo* shareInfo = (MemoryShareInfo*)kmalloc(sizeof(MemoryShareInfo));
    AtomicClearLock(&shareInfo->Lock);
    shareInfo->InitialSize = size;
    shareInfo->Type = type;
    shareInfo->RealSize = realSize;
    shareInfo->PageNumber = numberOfPage;
    shareInfo->Parent = process;
    shareInfo->PageTableParent = pageTable;
    shareInfo->VirtualAddressParent = virtualAddress;
    shareInfo->SlavesList = KStackInitialize(0x50);
    shareInfo->SlavesNumber = NULL;
    shareInfo->Offset = offset;
    shareInfo->signature0 = 'S';
    shareInfo->signature1 = 'M';

    *virtualAddressPointer = (uint64_t)virtualAddress;
    *keyPointer = (uint64_t)shareInfo;

    return KSUCCESS;
}

uint64_t MMAcceptMemoryField(kthread_t* self, kprocess_t* process, MemoryShareInfo* shareInfo, uint64_t* virtualAddressPointer){
    pagetable_t pageTable = process->SharedPaging;

    if(shareInfo->signature0 != 'S' || shareInfo->signature1 != 'M') return KFAIL;

    AtomicAcquire(&shareInfo->Lock);

    void* virtualAddress = (void*)*virtualAddressPointer;

    
    switch(shareInfo->Type){
        case MemoryFieldTypeShareSpaceRW:{
            if((uint64_t)virtualAddress % PAGE_SIZE > 0){
                virtualAddress = (void*)((uint64_t)virtualAddress - (uint64_t)virtualAddress % PAGE_SIZE);
            }

            int Errno = 0;
            MemoryRegion_t* Region = MMAllocateRegionVM(process->MemoryManager, virtualAddress, shareInfo->RealSize, MAP_FIXED | MAP_SHARED, PROT_READ | PROT_WRITE | PROT_EXEC, &Errno);

            if(!Region && Errno != EEXIST){
                return KFAIL;
            }

            for(uint64_t i = 0; i < shareInfo->PageNumber; i++){
                uint64_t virtualAddressIterator = (uint64_t)virtualAddress + i * PAGE_SIZE;
                uint64_t virtualAddressParentIterator = (uint64_t)shareInfo->VirtualAddressParent + i * PAGE_SIZE;
                void* physicalAddressParentIterator = vmm_GetPhysical(shareInfo->PageTableParent, (void*)virtualAddressParentIterator);
                vmm_Map(pageTable, (void*)virtualAddressIterator, physicalAddressParentIterator, true, true, false);
                vmm_SetFlags(pageTable, (void*)virtualAddressIterator, vmm_flag::vmm_Slave, true); //set slave state
            }

            break;
        }
        case MemoryFieldTypeShareSpaceRO:{
            if((uint64_t)virtualAddress % PAGE_SIZE > 0){
                virtualAddress = (void*)((uint64_t)virtualAddress - (uint64_t)virtualAddress % PAGE_SIZE);
            }

            int Errno = 0;
            MemoryRegion_t* Region = MMAllocateRegionVM(process->MemoryManager, virtualAddress, shareInfo->RealSize, MAP_FIXED | MAP_SHARED, PROT_READ | PROT_WRITE | PROT_EXEC, &Errno);

            if(!Region && Errno != EEXIST){
                return KFAIL;
            }

            for(uint64_t i = 0; i < shareInfo->PageNumber; i++){
                uint64_t virtualAddressIterator = (uint64_t)virtualAddress + i * PAGE_SIZE;
                uint64_t virtualAddressParentIterator = (uint64_t)shareInfo->VirtualAddressParent + i * PAGE_SIZE;
                void* physicalAddressParentIterator = vmm_GetPhysical(shareInfo->PageTableParent, (void*)virtualAddressParentIterator);
                vmm_Map(pageTable, (void*)virtualAddressIterator, physicalAddressParentIterator, true, false, false);
                vmm_SetFlags(pageTable, (void*)virtualAddressIterator, vmm_flag::vmm_Slave, true); //set slave state
            }
            break;
        }
        case MemoryFieldTypeSendSpaceRO:{
            pagetable_t lastPageTable = vmm_GetPageTable();
            vmm_Swap(self, pageTable);

            uint64_t alignement = (uint64_t)virtualAddress & 0xFFF;
            uint64_t size = shareInfo->RealSize;
            uint64_t pages = DivideRoundUp(size + alignement, PAGE_SIZE);

            void* virtualAddressAlign = virtualAddress;
            size_t sizeAlign = size;

            if((uint64_t)virtualAddressAlign % PAGE_SIZE > 0){
                virtualAddressAlign = (void*)((uint64_t)virtualAddressAlign - (uint64_t)virtualAddressAlign % PAGE_SIZE);
            }

            if(sizeAlign % PAGE_SIZE){
                sizeAlign -= sizeAlign % PAGE_SIZE;
                sizeAlign += PAGE_SIZE;
            }
            
            int Errno = 0;
            MemoryRegion_t* Region = MMAllocateRegionVM(process->MemoryManager, virtualAddressAlign, sizeAlign, MAP_FIXED | MAP_SHARED, PROT_READ | PROT_WRITE | PROT_EXEC, &Errno);

            if(!Region && Errno != EEXIST){
                return KFAIL;
            }

            if(Region){
                /* Allocate child memory */
                if((uint64_t)virtualAddress + pages * PAGE_SIZE < vmm_HHDMAdress){
                    for(uint64_t i = 0; i < pages; i++){
                        if(!vmm_GetFlags(pageTable, (void*)((uint64_t)virtualAddress + i * PAGE_SIZE), vmm_flag::vmm_Present)){
                            vmm_Unmap(pageTable, (void*)((uint64_t)virtualAddress + i * PAGE_SIZE));
                        }
                    }
                    
                    for(uint64_t i = 0; i < pages; i++){
                        if(!vmm_GetFlags(pageTable, (void*)((uint64_t)virtualAddress + i * PAGE_SIZE), vmm_flag::vmm_Present)){
                            vmm_Map(pageTable, (void*)((uint64_t)virtualAddress + i * PAGE_SIZE), Pmm_RequestPage(), true, true, false);
                            vmm_SetFlags(pageTable, (void*)((uint64_t)virtualAddress + i * PAGE_SIZE), vmm_flag::vmm_Master, true); //set master state
                            process->MemoryAllocated += PAGE_SIZE;                    
                        }
                    } 
                }
            }
            
            /* Copy memory */
            uint64_t virtualAddressParentIterator = (uint64_t)shareInfo->VirtualAddressParent;
            uint64_t virtualAddressIterator = (uint64_t)virtualAddress;
            uint64_t i = 0;

            if(virtualAddressParentIterator % PAGE_SIZE){
                uint64_t sizeToCopy = 0;
                if(size > PAGE_SIZE - (virtualAddressParentIterator % PAGE_SIZE)){
                    sizeToCopy = PAGE_SIZE - (virtualAddressParentIterator % PAGE_SIZE);
                }else{
                    sizeToCopy = size;
                }

                void* physicalAddressParentIterator = vmm_GetPhysical(shareInfo->PageTableParent, (void*)virtualAddressParentIterator);
                memcpy((void*)virtualAddressIterator, (void*)vmm_GetVirtualAddress(physicalAddressParentIterator), sizeToCopy);

                virtualAddressParentIterator += sizeToCopy;
                virtualAddressIterator += sizeToCopy;
                size -= sizeToCopy;
                i++;
            }

            while(size != 0){
                uint64_t sizeToCopy = size;
                if(size > PAGE_SIZE){
                    sizeToCopy = PAGE_SIZE;
                }

                void* physicalAddressParentIterator = vmm_GetPhysical(shareInfo->PageTableParent, (void*)virtualAddressParentIterator);

                memcpy((void*)virtualAddressIterator, (void*)vmm_GetVirtualAddress(physicalAddressParentIterator), sizeToCopy);
                virtualAddressIterator += sizeToCopy;
                virtualAddressParentIterator += sizeToCopy;
                size -= sizeToCopy;
            } 
            vmm_Swap(self, lastPageTable);
            break;
        }
    }

    SlaveInfo_t* SlaveInfo = (SlaveInfo_t*)kmalloc(sizeof(SlaveInfo_t));
    SlaveInfo->process = process;
    SlaveInfo->virtualAddress = virtualAddress;
    shareInfo->SlavesList->push64((uint64_t)SlaveInfo);
    shareInfo->SlavesNumber++;

    AtomicRelease(&shareInfo->Lock);
    
    *virtualAddressPointer = (uint64_t)virtualAddress + shareInfo->Offset;
    return KSUCCESS;
}

uint64_t MMCloseMemoryField(kthread_t* self, kprocess_t* process, MemoryShareInfo* shareInfo, void* virtualAddress){
    pagetable_t pageTable = process->SharedPaging;
    bool IsParent = (process == shareInfo->Parent);

    if(CheckAddress(virtualAddress, shareInfo->PageNumber * PAGE_SIZE) != KSUCCESS) return KFAIL;

    if(IsParent){
        for(uint64_t i = 0; i < shareInfo->SlavesNumber; i++){
            SlaveInfo_t* SlaveInfo = (SlaveInfo_t*)shareInfo->SlavesList->pop64();
            MMCloseMemoryField(self, SlaveInfo->process, shareInfo, SlaveInfo->virtualAddress);
        }        
    }

    AtomicAcquire(&shareInfo->Lock);

    switch(shareInfo->Type){
        case MemoryFieldTypeShareSpaceRW:{
            pagetable_t pageTableMaster = shareInfo->PageTableParent;
            size64_t NumberOfPage = shareInfo->PageNumber;
            for(uint64_t i = 0; i < NumberOfPage; i++){
                uint64_t virtualAddressIterator = (uint64_t)virtualAddress + i * PAGE_SIZE;
                if(vmm_GetFlags(pageTable, (void*)virtualAddressIterator, vmm_flag::vmm_Master)){ // is master
                    void* physcialAddress = vmm_GetPhysical(pageTable, (void*)virtualAddressIterator);
                    Pmm_FreePage(physcialAddress);  
                    process->MemoryAllocated -= PAGE_SIZE;      
                }
                vmm_Unmap(pageTable, (void*)virtualAddressIterator);
            }
            break;
        }
        case MemoryFieldTypeShareSpaceRO:{
            pagetable_t pageTableMaster = shareInfo->PageTableParent;
            size64_t NumberOfPage = shareInfo->PageNumber;
            for(uint64_t i = 0; i < NumberOfPage; i++){
                uint64_t virtualAddressIterator = (uint64_t)virtualAddress + i * PAGE_SIZE;
                if(vmm_GetFlags(pageTable, (void*)virtualAddressIterator, vmm_flag::vmm_Master)){ // is master
                    void* physcialAddress = vmm_GetPhysical(pageTable, (void*)virtualAddressIterator);
                    Pmm_FreePage(physcialAddress);  
                    process->MemoryAllocated -= PAGE_SIZE;      
                }
                vmm_Unmap(pageTable, (void*)virtualAddressIterator);
            }
            break;
        }
        case MemoryFieldTypeSendSpaceRO:{
            break;
        }
    }
    
    AtomicRelease(&shareInfo->Lock);
    
    if(IsParent){
        kfree((void*)shareInfo);
    }
    
    return KSUCCESS;
}