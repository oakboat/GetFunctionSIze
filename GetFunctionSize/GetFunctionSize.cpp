#include <stdio.h>
#include <inttypes.h>
#include "Zydis.h"
#include <Windows.h>

size_t GetFuncSIze(void * addr)
{
    ZyanU8* data = reinterpret_cast<ZyanU8*>(addr);
    // The runtime address (instruction pointer) was chosen arbitrarily here in order to better
    // visualize relative addressing. In your actual program, set this to e.g. the memory address
    // that the code being disassembled was read from.
    ZyanU64 runtime_address = reinterpret_cast<ZyanU64>(data);
    // Loop over the instructions in our buffer.
    ZyanUSize offset = 0;
    ZydisDisassembledInstruction instruction;
    while (ZYAN_SUCCESS(ZydisDisassembleIntel(
        /* machine_mode:    */ ZYDIS_MACHINE_MODE_LONG_64,
        /* runtime_address: */ runtime_address,
        /* buffer:          */ data + offset,
        /* length:          */ 15,
        /* instruction:     */ &instruction
    ))) {
        printf("%016" PRIX64 "  %s\n", runtime_address, instruction.text);
        offset += instruction.info.length; //count instruction length
        runtime_address += instruction.info.length;
        if (instruction.info.opcode == 0xc3) //find ret, then break
        {
            return offset;
        }
    }
    return 0;
}

void* GetRealFunc(void* addr)
{
    ZyanU8* data = reinterpret_cast<ZyanU8*>(addr);
    // The runtime address (instruction pointer) was chosen arbitrarily here in order to better
    // visualize relative addressing. In your actual program, set this to e.g. the memory address
    // that the code being disassembled was read from.
    ZyanU64 runtime_address = reinterpret_cast<ZyanU64>(data);
    ZydisDisassembledInstruction instruction;
    if (ZYAN_SUCCESS(ZydisDisassembleIntel(
        /* machine_mode:    */ ZYDIS_MACHINE_MODE_LONG_64,
        /* runtime_address: */ runtime_address,
        /* buffer:          */ data,
        /* length:          */ 15,
        /* instruction:     */ &instruction
    ))) {
        printf("%016" PRIX64 "  %s\n", runtime_address, instruction.text); // get jmp code
        return reinterpret_cast<void*>(runtime_address + instruction.info.length + instruction.operands[0].imm.value.s); //get real address
    }
    return 0;
}

void TestFunc(decltype(&MessageBoxA) msg, const char* str)
{
    msg(nullptr, str, str, MB_OK); //because call instruction uses relative addressing, we need to manually pass in the function address.
}

int main()
{
#ifdef _DEBUG
    auto addr = GetRealFunc(TestFunc); //bypass jmp and get real function address
#else
    auto addr = TestFunc;
#endif // DEBUG
    auto size = GetFuncSIze(addr); //get function size
    auto running_addr = VirtualAlloc(nullptr, size, MEM_COMMIT, PAGE_READWRITE); // Allocate memory to put our function and run it
    memcpy(running_addr, addr, size); //copy our functiom
    DWORD dummy;
    VirtualProtect(running_addr, size, PAGE_EXECUTE_READ, &dummy); // make memory excutable
    decltype(&TestFunc) func_ptr;
    func_ptr = reinterpret_cast<decltype(func_ptr)>(running_addr);
    func_ptr(MessageBoxA, "Hello World"); //excute memory
    return 0;
}
