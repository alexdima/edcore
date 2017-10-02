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
  private:
    BufferNode *root;

  public:
    Buffer(BufferNode *root);
    ~Buffer();
    size_t length() const;
    size_t getLineCount() const;
    void print(ostream &os);

    bool findOffset(size_t offset, BufferCursor &result);
    bool findLine(size_t lineNumber, BufferCursor &start, BufferCursor &end);
    void extractString(BufferCursor start, size_t len, uint16_t *dest);
};
}

#endif
