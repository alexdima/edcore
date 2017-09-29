#ifndef SRC_MEM_MANAGER_H_
#define SRC_MEM_MANAGER_H_

#include <memory>
#include <iostream>

using namespace std;

namespace edcore
{

class SimpleString;
class BufferString;
class BufferStringSubstring;
class BufferNode;
class Buffer;

class MemManager
{
  private:
    size_t _simpleStringCnt;
    size_t _bufferStringCnt;
    size_t _bufferStringSubstringCnt;
    size_t _bufferNodeCnt;
    size_t _bufferCnt;

    MemManager();

  public:
    static MemManager &getInstance();

    void _register(SimpleString *simpleString);
    void _unregister(SimpleString *simpleString);

    void _register(BufferString *bufferString);
    void _unregister(BufferString *bufferString);

    void _register(BufferStringSubstring *bufferStringSubstring);
    void _unregister(BufferStringSubstring *bufferStringSubstring);

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
