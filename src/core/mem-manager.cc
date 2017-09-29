/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "mem-manager.h"

namespace edcore {

MemManager::MemManager()
{
    this->cntBufferString_ = 0;
    this->cntBufferNode_ = 0;
    this->cntBuffer_ = 0;
}

MemManager &MemManager::getInstance()
{
    static MemManager mm;
    return mm;
}

void MemManager::_register(BufferString *bufferString)
{
    this->cntBufferString_++;
}
void MemManager::_unregister(BufferString *bufferString)
{
    this->cntBufferString_--;
}

void MemManager::_register(BufferNode *bufferNode)
{
    this->cntBufferNode_++;
}
void MemManager::_unregister(BufferNode *bufferNode)
{
    this->cntBufferNode_--;
}

void MemManager::_register(Buffer *buffer)
{
    this->cntBuffer_++;
}
void MemManager::_unregister(Buffer *buffer)
{
    this->cntBuffer_--;
}

void MemManager::print(ostream &os) const
{
    os << "MM [" << endl;
    os << "  -> bufferStringCnt: " << this->cntBufferString_ << endl;
    os << "  -> bufferNodeCnt: " << this->cntBufferNode_ << endl;
    os << "  -> bufferCnt: " << this->cntBuffer_ << endl;
    os << "]" << endl;
}

}
