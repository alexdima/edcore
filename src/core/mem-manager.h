/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef EDCORE_MEM_MANAGER_H_
#define EDCORE_MEM_MANAGER_H_

#include <memory>
#include <iostream>

using namespace std;

namespace edcore
{

class BufferNodeString;
class BufferNode;
class Buffer;

class MemManager
{
  private:
    size_t cntBufferNodeString_;
    size_t cntBufferNode_;
    size_t cntBuffer_;

    MemManager();

  public:
    static MemManager &getInstance();

    void _register(BufferNodeString *bufferString);
    void _unregister(BufferNodeString *bufferString);

    void _register(BufferNode *bufferNode);
    void _unregister(BufferNode *bufferNode);

    void _register(Buffer *buffer);
    void _unregister(Buffer *buffer);

    void print(ostream &os) const;
};

#ifdef TRACK_MEMORY

#define MM_REGISTER(what) MemManager::getInstance()._register(this)
#define MM_UNREGISTER(what) MemManager::getInstance()._unregister(this)
#define MM_DUMP(os) MemManager::getInstance().print(os)

#else

#define MM_REGISTER(what)
#define MM_UNREGISTER(what)
#define MM_DUMP(os) os << "Memory not tracked" << endl

#endif

}

#endif
