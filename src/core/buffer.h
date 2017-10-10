/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

#ifndef EDCORE_BUFFER_H_
#define EDCORE_BUFFER_H_

#include "buffer-piece.h"
#include "buffer-string.h"

#include <memory>
#include <vector>

namespace edcore
{

size_t log2(size_t n);

struct BufferNode
{
    size_t length;
    size_t newLineCount;
};
typedef struct BufferNode BufferNode;

struct BufferCursor
{
    size_t offset;
    size_t leafIndex;
    size_t leafStartOffset;
};
typedef struct BufferCursor BufferCursor;

struct OffsetLenEdit
{
    size_t initialIndex;
    size_t offset;
    size_t length;
    const uint16_t *data;
    size_t dataLength;
};
typedef struct OffsetLenEdit OffsetLenEdit;

struct OffsetLenEdit2
{
    size_t initialIndex;
    size_t offset;
    size_t length;
    const BufferString *text;
};
typedef struct OffsetLenEdit2 OffsetLenEdit2;

struct InternalOffsetLenEdit
{
    size_t startLeafIndex;
    size_t startInnerOffset;
    size_t endLeafIndex;
    size_t endInnerOffset;
    const uint16_t *data;
    size_t dataLength;
};
typedef struct InternalOffsetLenEdit InternalOffsetLenEdit;

struct InternalOffsetLenEdit2
{
    size_t startLeafIndex;
    size_t startInnerOffset;
    size_t endLeafIndex;
    size_t endInnerOffset;
    const BufferString *text;
};
typedef struct InternalOffsetLenEdit2 InternalOffsetLenEdit2;


// struct InternalLeafOffsetLenEdit
// {
//     size_t leafIndex;
//     size_t start;
//     size_t length;
//     const BufferString *text;
// };
// typedef struct InternalLeafOffsetLenEdit InternalLeafOffsetLenEdit;

struct LeafReplacement {
    size_t startLeafIndex;
    size_t endLeafIndex;
    vector<BufferPiece*> *replacements;
};
typedef struct LeafReplacement LeafReplacement;

class Buffer
{
  public:
    Buffer(vector<BufferPiece *> &pieces, size_t minLeafLength, size_t maxLeafLength);
    ~Buffer();
    size_t length() const { return nodes_[1].length; }
    size_t lineCount() const { return nodes_[1].newLineCount + 1; }
    size_t memUsage() const;

    bool findOffset(size_t offset, BufferCursor &result);
    bool findLine(size_t lineNumber, BufferCursor &start, BufferCursor &end);
    void extractString(BufferCursor start, size_t len, uint16_t *dest);

    void deleteOneOffsetLen(size_t offset, size_t len);
    void insertOneOffsetLen(size_t offset, const uint16_t *data, size_t len);
    void replaceOffsetLen(vector<OffsetLenEdit> &edits);
    void replaceOffsetLen(vector<OffsetLenEdit2> &edits);

    void assertInvariants();
    void assertNodeInvariants(size_t nodeIndex);

  private:
    BufferNode *nodes_;
    size_t nodesCount_;

    MyArray<BufferPiece*> leafs_;
    size_t leafsStart_;
    size_t leafsEnd_;

    size_t minLeafLength_;
    size_t maxLeafLength_;
    size_t idealLeafLength_;

    bool _findLineStart(size_t &lineIndex, BufferCursor &result);
    void _findLineEnd(size_t leafIndex, size_t leafStartOffset, size_t innerLineIndex, BufferCursor &result);
    void _updateNodes(size_t fromNodeIndex, size_t toNodeIndex);
    void _updateSingleNode(size_t nodeIndex);
    void _rebuildNodes();
    size_t _nextNonEmptyLeafIndex(size_t leafIndex);

    void resolveEdits(vector<OffsetLenEdit2> &_edits, vector<InternalOffsetLenEdit2> &edits, vector<BufferString*> &toDelete);
    void flushLeafEdits(size_t accumulatedLeafIndex, vector<LeafOffsetLenEdit2> &accumulatedLeafEdits, vector<LeafReplacement> &replacements);
    void appendLeaf(BufferPiece *leaf, vector<BufferPiece*> &leafs, BufferPiece *&prevLeaf);
};
}

#endif
