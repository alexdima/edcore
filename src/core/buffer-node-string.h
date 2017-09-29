/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef SRC_BUFFER_NODE_STRING_H_
#define SRC_BUFFER_NODE_STRING_H_

#include <memory>

using namespace std;

namespace edcore
{

class BufferNodeString
{
  public:
    BufferNodeString(uint16_t *data, size_t len);
    ~BufferNodeString();

    const uint16_t *getData() const; // TODO
    size_t getLen() const;

    const size_t *getLineStarts() const;
    size_t getNewLineCount() const;

    void print(ostream &os) const;

  private:
    uint16_t *_data;
    size_t _len;

    size_t *_lineStarts;
    size_t _lineStartsCount;

    void _init(uint16_t *data, size_t len, size_t *lineStarts, size_t lineStartsCount);
};
}

#endif
