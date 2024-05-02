#include <stdio.h>
#include <stdint.h>
#include <unordered_map>
#include <string>

static constexpr uint16_t MBR_SIGNATURE = 0xAA55;
static constexpr uint32_t MBR_CODE_LENGTH_BYTES = 446;
static constexpr uint32_t MBR_NUM_PARTITION_ENTRIES = 4;

struct Partition
{
    uint8_t bootIndicator;
    uint8_t startCHS[3];
    uint8_t type;
    uint8_t endCHS[3];
    uint32_t startSectorLBA;
    uint32_t sizeInSectors;
};

static_assert(sizeof(Partition) == 16);

const std::unordered_map<uint8_t, std::string> PartitionTypes = 
{
    { 0x00, "empty" },
    { 0x01, "FAT12" },
    { 0x04, "FAT16 small" },
    { 0x05, "Extended" },
    { 0x06, "FAT16 big" },
    { 0x0b, "FAT32" },
    { 0x0c, "FAT32/INT13" },
    { 0x0e, "FAT16/INT13" },
    { 0x0f, "Extended/INT13" }
};

static inline uint32_t DecodeCHSCylinder(uint8_t chs[3])
{
    uint32_t cylinder = (uint32_t)chs[2] | ((uint32_t)(chs[1] & (0x80 | 0x40))) << 2;
    return cylinder;
}

static inline uint32_t DecodeCHSHead(uint8_t chs[3])
{
    return chs[0];
}

static inline uint32_t DecodeCHSSector(uint8_t chs[3])
{
    uint32_t sector = chs[1] & 0x3f;
    return sector;
}

int main(int argc, char** argv)
{
    if (argc != 2)
    {
        printf("usage: partinfo <filename.img>\n");
        return 1;
    }

    FILE* fp = nullptr;
    fopen_s(&fp, argv[1], "rb");
    if (fp == nullptr)
    {
        printf("failed to open '%s'\n", argv[1]);
        return 1;
    }

    uint8_t buffer[512];
    if (fread_s(buffer, sizeof(buffer), 1, sizeof(buffer), fp) != sizeof(buffer))
    {
        printf("failed to read MBR\n");
        fclose(fp);
        return 1;
    }

    fclose(fp);

    uint16_t signature = *(uint16_t*)(&buffer[510]);
    if (signature != MBR_SIGNATURE)
    {
        printf("bad signature in MBR (expected 0x%X, read 0x%X)\n", MBR_SIGNATURE, signature);
        return 1;
    }

    Partition* table = (Partition*)&buffer[MBR_CODE_LENGTH_BYTES];
    printf("No.  Active  Type           Start CHS      End CHS        Start Sector (LBA)  Size (in sectors)\n");
    for (int i = 0; i < MBR_NUM_PARTITION_ENTRIES; i++, table++)
    {
        printf("%u    ", i);
        printf("   %c    ", table->bootIndicator ? '*' : ' ');

        const auto type = PartitionTypes.find(table->type);
        const std::string& typeStr = type == PartitionTypes.end() ? "unknown" : type->second;
        printf("%-14s ", typeStr.c_str());

        char chs[1024];
        sprintf_s(chs, "%u/%u/%u", DecodeCHSCylinder(table->startCHS), DecodeCHSHead(table->startCHS), DecodeCHSSector(table->startCHS));
        printf("%-14s ", chs);

        sprintf_s(chs, "%u/%u/%u", DecodeCHSCylinder(table->endCHS), DecodeCHSHead(table->endCHS), DecodeCHSSector(table->endCHS));
        printf("%-14s ", chs);

        printf("%-19u ", table->startSectorLBA);
        printf("%u\n", table->sizeInSectors);
    }

    return 0;
}
