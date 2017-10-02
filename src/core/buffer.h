/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef EDCORE_BUFFER_H_
#define EDCORE_BUFFER_H_

#include "buffer-node.h"

#include <memory>

namespace edcore
{

class Buffer
{
  public:
    Buffer(BufferNode *root);
    ~Buffer();
    size_t length() const { return this->root->length(); }
    size_t lineCount() const { return this->root->newLinesCount() + 1; }
    void print(ostream &os);

    bool findOffset(size_t offset, BufferCursor &result);
    bool findLine(size_t lineNumber, BufferCursor &start, BufferCursor &end);
    void extractString(BufferCursor start, size_t len, uint16_t *dest);

  private:
    BufferNode *root;
};
}

#endif
