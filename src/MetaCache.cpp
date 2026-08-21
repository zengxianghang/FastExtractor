#include "MetaCache.h"
#include <fstream>

bool saveMeta(const char* filename, const FileMeta& meta)
{
    std::ofstream fout(filename);
    if(!fout)
        return false;

    fout << meta.fileSize << "\n";
    fout << meta.start.week << " " << meta.start.sow << "\n";
    fout << meta.end.week << " " << meta.end.sow << "\n";
    fout << (meta.valid ? 1 : 0) << "\n";

    return true;
}

bool loadMeta(const char* filename, FileMeta& meta)
{
    std::ifstream fin(filename);
    if(!fin)
        return false;

    int valid;

    fin >> meta.fileSize;
    fin >> meta.start.week >> meta.start.sow;
    fin >> meta.end.week >> meta.end.sow;
    fin >> valid;

    meta.valid = (valid != 0);

    return true;
}
