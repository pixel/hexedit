#include "hexedit.h"
#include <uchar.h>


size_t encoding_parse_one(uint8_t *buffer, encodingEntry* entry);

encodingEntry** encoding_load(char *encodingFile, int* count) {
    int handle = open(encodingFile, O_RDONLY);
    if (handle == -1)
        DIE("%s: Failed to open encoding file\n")

    struct stat nodeInfo;
    fstat(fd, &nodeInfo);
    off_t size = nodeInfo.st_size;

    void *buffer = malloc(size);
    if (buffer == NULL)
        DIE("%s: Malloc failed for encoding file buffer\n")

    int readBytes = read(handle, buffer, size);

    return encoding_parse(buffer, count, size);
}

encodingEntry** encoding_parse(uint8_t *buffer, int* count, size_t length) {
    uint8_t* position = buffer;
    encodingEntry** entries = calloc(32, sizeof(encodingEntry *));
    int entryCount = 0;
    do {
        // Fix up pointer being left on the separator
        if (entryCount != 0)
            position++;
        encodingEntry* entry = malloc(sizeof(encodingEntry));
        position += encoding_parse_one(position, entry);
        entries[entryCount++] = entry;
        entries[entryCount] = 0;
        //printf("After reading encoding: %02x %08x/%08x (%d)\n", *position, position, buffer + length, length);
    } while (position < buffer + length && *position == 0x1C);

    *count = entryCount;

    return entries;
}

size_t encoding_parse_one(uint8_t *buffer, encodingEntry* entry) {
    entry->name = strndup(buffer, 256);
    uint8_t* position = buffer + strnlen(entry->name, 256);

    printf("Encoding registered: \"%s\"\n", entry->name);
    if (*(position++) != 0x00)
        DIE("%s: Invalid encoding file, missing null terminator\n")
    if (*(position++) != 0x1D)
        DIE("%s: Invalid encoding file, missing name record mark\n")

    // Doesn't cover worst case: ZWJ family emoji in all cells
    size_t displayCharLength = strnlen(position, 2048);
    entry->displayCharacters = calloc(256, sizeof(char8_t*));

    wchar_t wide = '.';
    char8_t* displayPtr = calloc((displayCharLength + 256), sizeof(char8_t));
    //printf("Char storage length: %lu, %08X - %08X\n", displayCharLength, displayPtr, displayPtr + displayCharLength + 256 - 1);

    // This does not handle grapheme clusters at all, just the ZWJ emoji
    // Meaning flags get chopped up weirdly!
    // Oh and any script that has modifying characters. And no, "Nobody would use that" is not something to say
    for (int i = 0; i < 256; i++) {
        entry->displayCharacters[i] = displayPtr;
        bool foundJoiner = false;
        do {
            size_t len = mbtowc(&wide, position, 4);
            if (memccpy(displayPtr, position, 0, len) != NULL)
                DIE("%s: Tried to read past null byte?");
            position += len;
            displayPtr += len;

            size_t next = mbtowc(&wide, position, 4);
            // Does this care if ZWJ sequence is valid or not? Meh
            // Best effort given here, so...
            if (next == -1 || wide != 0x200D)
                break;
            foundJoiner = true;
            if (memccpy(displayPtr, position, 0, next) != NULL)
                DIE("%s: Tried to read more past null byte?");
            position += next;
            displayPtr += next;
        } while (foundJoiner);
        if (entry->displayCharacters[i] == displayPtr)
            DIE("%s: Empty display character, is something off by one?\n");
        *displayPtr = 0;
        //printf("Char %02x = \"%s\" (%08X - %08X)\n", i, entry->displayCharacters[i], entry->displayCharacters[i], displayPtr);
        displayPtr++;
    }
    if (*(position++) != 0x00)
        DIE("%s: Invalid encoding file, missing null terminator\n")
    if (*(position++) != 0x1D)
        DIE("%s: Invalid encoding file, missing display characters record mark\n")

    // TODO: stub for handling color maps

    if (*(position++) != 0x1D)
        DIE("%s: Invalid encoding file, missing color map record mark\n")

    return position - buffer;
}