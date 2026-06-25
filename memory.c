#include <stdlib.h>

#include "compiler.h"
#include "memory.h"
#include "object.h"
#include "vm.h"

#ifdef DEBUG_LOG_GC
#include <stdio.h>
#include "debug.h"
#endif

// Multiple of the heap size to scale the threshold in which the GC is run.
#define GC_HEAP_GROW_FACTOR 2

void* reallocate(void* pointer, size_t oldSize, size_t newSize)
{
    // Adjust the number of bytes allocated by the VM, when a reallocation has been made.
    vm.bytesAllocated += newSize - oldSize;
    if (newSize > oldSize)
    {
        // Force a marking stage to occur every time memory is allocated.
        #ifdef DEBUG_STRESS_GC
        collectGarbage();
        #endif
    }

    // If the VM has exceeded the number of bytes we can allocate before triggering the GC, then run the GC.
    if (vm.bytesAllocated > vm.nextGC)
    {
        collectGarbage();
    }

    // If the new size requested is zero, free the memory block and clear the pointer.
    if (newSize == 0)
    {
        free(pointer);
        return NULL;
    }

    // Resize the existing dynamic memory block to match the new size specification given in the header file. 0
    void* result = realloc(pointer, newSize);
    // Re-allocating the dynamic array has failed.
    if (result == NULL) exit(1);
    return result;
}

void markObject(Obj* object)
{
    if (object == NULL) return;
    // Ensure we do not remain in an infinite loop of circular pointer references.
    if (object -> isMarked) return;

    #ifdef DEBUG_LOG_GC
        printf("%p mark ", (void*)object);
        printValue(OBJ_VAL(object));
        printf("\n");
    #endif

    object -> isMarked = true;

    // Produce a working list of the grey objects within the traversal. [Grey => Object is reachable but we have not traced the objects it references.]
        // Check if the current garbage collection grey stack has reached its maximum allocated capacity limits.
    if (vm.grayCapacity < vm.grayCount + 1)
    {
        // Calculate the next expanded sizing threshold for the tracking array buffer dynamically using the growth factor.
        vm.grayCapacity = GROW_CAPACITY(vm.grayCapacity);
        // Resize the underlying heap memory block allocated for the grey object pointer stack array tracking storage.
        vm.grayStack = (Obj**)realloc(vm.grayStack, sizeof(Obj*) * vm.grayCapacity);
    }


    if (vm.grayStack == NULL) exit(1);
    // Append the newly discovered grey object pointer directly onto the working list stack buffer and increment tracking totals.
    vm.grayStack[vm.grayCount++] = object;
}

void markValue(Value value)
{
    if (IS_OBJ(value)) markObject(AS_OBJ(value));
}

static void markArray(ValueArray* array)
{
    for (int i = 0; i < array -> count; i++)
    {
        markValue(array -> values[i]);
    }
}

// Convert a given object to black and turn its references grey.
static void blackenObject(Obj* object)
{
    #ifdef DEBUG_LOG_GC
        printf("%p blacken ", (void*)object);
        printValue(OBJ_VAL(object));
        printf("\n");
    #endif
    switch (object -> type)
    {
        case OBJ_CLOSURE:
        {
            ObjClosure* closure = (ObjClosure*)object;
            markObject((Obj*)closure -> function);
            for (int i = 0; i < closure -> upvalueCount; i++)
            {
                markObject((Obj*)closure -> upvalues[i]);
            }
            break;
        }
        case OBJ_FUNCTION:
        {
            ObjFunction* function = (ObjFunction*) object;
            markObject((Obj*)function -> name);
            markArray(&function -> chunk.constants); // Also mark all of the references made by the function.
            break;
        }
        case OBJ_UPVALUE:
            // An upvalue has no outgoing references and can hence be marked black and the traversal can keep going.
            markValue(((ObjUpvalue*)object) -> closed);
            break;
        case OBJ_NATIVE:
        case OBJ_STRING:
            break;
    }
}

// Free a singular dynamically allocated object.
static void freeObject(Obj* object)
{
    #ifdef DEBUG_LOG_GC
        printf("%p free type %d\n", (void*)object, object -> type);
    #endif // Log memory information for the GC.

    // Inspects the object's type tag to determine the correct cleanup logic.
    switch(object -> type)
    {
        case OBJ_CLOSURE:
        {
            // Free the associated upvalues with the closure.
            ObjClosure* closure =(ObjClosure*)object;
            FREE_ARRAY(ObjUpvalue*, closure -> upvalues, closure -> upvalueCount);
            // Only free the closure and not the function because the closure does not own the function. I.e. there may be multiple closures over the same function.
            FREE(ObjClosure, object);
            break;
        }
        case OBJ_FUNCTION:
        {
            // Safely casts the base Obj pointer back to its specific ObjFunction type.
            ObjFunction* function = (ObjFunction*)object;
            // Frees the chunk of byte-code representing the function body as well as the other meta-data.
            // The function name is handled by the GC and so does not need to be explicitly freed here.
            freeChunk(&function -> chunk);
            FREE(ObjFunction, object);
            break;
        }
        case OBJ_NATIVE:
            FREE(ObjNative, object);
            break;
        case OBJ_STRING:
        {
            // Safely casts the base Obj pointer back to its specific ObjString type.
            ObjString* string = (ObjString*)object;

            // Frees the heap-allocated character array, including its null terminator.
            FREE_ARRAY(char, string -> chars, string -> length + 1);

            // Frees the ObjString structure itself from heap memory.
            FREE(ObjString, object);
            break;
        }
        case OBJ_UPVALUE:
            FREE(ObjUpvalue, object);
            break;
    }
}

// Free all dynamically allocated objects stored within the VM linked list.
void freeObjects()
{
    Obj* object = vm.objects;
    while (object != NULL)
    {
        Obj* next = object -> next;
        freeObject(object);
        object = next;
    }
    free(vm.grayStack);
}

static void markRoots()
{
    // Most roots are within the stack and so we can simply walk through it and mark most of the values directly.
    for(Value* slot = vm.stack; slot < vm.stackTop; slot++)
    {
        markValue(*slot)
    }

    // Mark the call stack frames maintained by the VM.
    for (int i = 0; i < vm.frameCount; i++)
    {
        markObject((Obj*)vm.frames[i].closure);
    }

    // Mark the linked list of upvalues that is maintained by the VM.
    for (ObjUpvalue* upvalue = vm.openUpvalues; upvalue != NULL, upvalue = upvalue -> next)
    {
        markObject((Obj*)upvalue);
    }

    // Common source of roots are global variables so mark them for sweeping.
    markTable(&vm.globals);
    // The compiler itself also produces values on the heap / constant table. Therefore it should also be marked.
    markCompilerRoots();
}

static void traceReferences()
{
    while (vm.grayCount > 0)
    {
        Obj* object = vm.grayStack[--vm.grayCount];
        blackenObject(object);
    }
}

static void sweep()
{
    // Tracks the immediately preceding object node in the global memory linked list during iteration.
    Obj* previous = NULL;
    // Start scanning from the head of the VM's global linked list of dynamically allocated objects.
    Obj* object = vm.objects;
    // Iterate through every object on the heap until reaching the end of the memory tracking list.
    while (object != NULL)
    {
        // If the object was marked as reachable during the mark phase, preserve it and clear it for the next collection cycle.
        if (object -> isMarked)
        {
            object -> isMarked = false;
            previous = object;
            object = object -> next;
        }
        else
        {
            // Capture a pointer to the unreachable object to isolate it before updating the linked list connections.
            Obj* unreached = object;
            // Advance the tracking pointer to the next object in line before deleting the current node.
            object = object -> next;
            // Re-link the preceding node directly to the subsequent node, skipping over the isolated object.
            if (previous != NULL)
            {
                previous -> next = object;
            }
            else
            {
                // If removing the head of the list, update the global VM object pointer to point to the next node.
                vm.objects = object;
            }
            // Release the heap-allocated memory blocks associated with the un-reached object and all its internal sub-allocations.
            freeObject(unreached);
        }
    }
}

// Mark the unreachable sections of memory for clearing by the garbage collector.
void collectGarbage()
{
    #ifdef DEBUG_LOG_GC
        printf("-- gc begin\n");
        size_t before = vm.bytesAllocated;
    #endif // Log the garbage collector if enabled.

    // Indicate the objects to be cleared by the GC in the sweeping stage.
    markRoots();
    // Trace the references of the marked objects to find the reachable nodes. [Gray => reachable but not expanded, black => reachable and expanded, white => has not been reached yet]
    traceReferences();
    tableRemoveWhite(&vm.strings);
    // Sweep the white objects (that have been unmarked) as they are unreachable and can hence be freed, without compromising the correctness / efficiency of the program.
    sweep();

    // Adjust the threshold of the next run of the GC to ensure it is run at the right frequency.
    vm.nextGC = vm.bytesAllocated * GC_HEAP_GROW_FACTOR;
    #ifdef DEBUG_LOG_GC
        printf("-- gc end\n");
        printf("   collected %zu bytes (from %zu to %zu) next at %zu\n", before - vm.bytesAllocated, before, vm.bytesAllocated, vm.nextGC);
    #endif
}
