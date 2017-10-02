/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "edcore.h"

#include <iostream>
// #include <memory>

// #include <fstream>
#include <vector>
// #include <cstring>
// #include <stdint.h>
#include <assert.h>
// #include <stdlib.h>

#include "mem-manager.h"

using namespace std;

#define PIECE_SIZE 4 * 1024 // 4KB

namespace edcore
{

class BufferNodeString;
class BufferNode;
class Buffer;









// Buffer *buildBufferFromFile(const char *filename)
// {
//     ifstream ifs(filename, ifstream::binary);
//     if (!ifs)
//     {
//         return NULL;
//     }
//     vector<shared_ptr<BufferNodeString>> rawPieces;
//     ifs.seekg(0, std::ios::beg);
//     while (!ifs.eof())
//     {
//         char *piece = new char[PIECE_SIZE];

//         ifs.read(piece, PIECE_SIZE);

//         if (ifs)
//         {
//             rawPieces.push_back(shared_ptr<BufferNodeString>(new BufferNodeString(piece, PIECE_SIZE)));
//         }
//         else
//         {
//             rawPieces.push_back(shared_ptr<BufferNodeString>(new BufferNodeString(piece, ifs.gcount())));
//         }
//     }
//     ifs.close();

//     size_t pieceCount = rawPieces.size();
//     BufferNode *root = buildBufferFromPieces(rawPieces, 0, pieceCount);
//     return new Buffer(root);
// }

timespec diff(timespec start, timespec end)
{
    timespec temp;
    if ((end.tv_nsec - start.tv_nsec) < 0)
    {
        temp.tv_sec = end.tv_sec - start.tv_sec - 1;
        temp.tv_nsec = 1000000000 + end.tv_nsec - start.tv_nsec;
    }
    else
    {
        temp.tv_sec = end.tv_sec - start.tv_sec;
        temp.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return temp;
}

double took(timespec start, timespec end)
{
    timespec d = diff(start, end);
    return d.tv_sec * 1000 + d.tv_nsec / (double)1000000;
}

timespec _tmp_timespec;

#define TIME_START(name) \
    timespec name;       \
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &name)

#define TIME_END(os, name, explanation)                      \
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &_tmp_timespec); \
    os << explanation << " took " << took(name, _tmp_timespec) << " ms." << endl
}
