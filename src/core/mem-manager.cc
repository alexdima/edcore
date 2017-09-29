/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#include "mem-manager.h"

namespace edcore {

MemManager::MemManager()
{
    this->_bufferStringCnt = 0;
    this->_bufferNodeCnt = 0;
    this->_bufferCnt = 0;
}

MemManager &MemManager::getInstance()
{
    static MemManager mm;
    return mm;
}

void MemManager::_register(BufferString *bufferString)
{
    this->_bufferStringCnt++;
}
void MemManager::_unregister(BufferString *bufferString)
{
    this->_bufferStringCnt--;
}

void MemManager::_register(BufferNode *bufferNode)
{
    this->_bufferNodeCnt++;
}
void MemManager::_unregister(BufferNode *bufferNode)
{
    this->_bufferNodeCnt--;
}

void MemManager::_register(Buffer *buffer)
{
    this->_bufferCnt++;
}
void MemManager::_unregister(Buffer *buffer)
{
    this->_bufferCnt--;
}

void MemManager::print(ostream &os) const
{
    os << "MM [" << endl;
    os << "  -> bufferStringCnt: " << this->_bufferStringCnt << endl;
    os << "  -> bufferNodeCnt: " << this->_bufferNodeCnt << endl;
    os << "  -> bufferCnt: " << this->_bufferCnt << endl;
    os << "]" << endl;
}

}
