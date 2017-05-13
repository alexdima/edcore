#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>

using namespace std;

#define PIECE_SIZE 4 * 1024 // 4KB

class SimpleString
{
  private:
    const char *data;
    const size_t len;

  public:
    SimpleString(char *_data, size_t _len)
        : data(_data), len(_len)
    {
    }

    const char *getData() const
    {
        return this->data;
    }

    const size_t getLen() const
    {
        return this->len;
    }
};

std::ostream &operator<<(std::ostream &os, SimpleString *const &m)
{
    if (m == NULL)
    {
        return os << "[NULL]";
    }
    const char *data = m->getData();
    for (size_t i = 0, len = m->getLen(); i < len; i++)
    {
        os << data[i];
    }
    return os;
}

class BufferString
{
  private:
    char *data;
    size_t len;

    size_t *lineStarts;
    size_t lineStartsCount;

  public:
    BufferString(char *data, size_t len)
    {
        assert(data != NULL && len != 0);

        // Do a first pass to count the number of line starts
        size_t lineStartsCount = 0;
        for (size_t i = 0; i < len; i++)
        {
            char chr = data[i];

            if (chr == '\r')
            {
                if (i + 1 < len && data[i + 1] == '\n')
                {
                    // \r\n... case
                    lineStartsCount++;
                    i++; // skip \n
                }
                else
                {
                    // \r... case
                    lineStartsCount++;
                }
            }
            else if (chr == '\n')
            {
                lineStartsCount++;
            }
        }

        size_t *lineStarts = new size_t[lineStartsCount];

        size_t dest = 0;
        for (size_t i = 0; i < len; i++)
        {
            char chr = data[i];

            if (chr == '\r')
            {
                if (i + 1 < len && data[i + 1] == '\n')
                {
                    // \r\n... case
                    lineStarts[dest++] = i + 2;
                    i++; // skip \n
                }
                else
                {
                    // \r... case
                    lineStarts[dest++] = i + 1;
                }
            }
            else if (chr == '\n')
            {
                lineStarts[dest++] = i + 1;
            }
        }
        this->_init(data, len, lineStarts, lineStartsCount);
    }

    BufferString(char *data, size_t len, size_t *lineStarts, size_t lineStartsCount)
    {
        assert(data != NULL && len != 0);
        assert(lineStarts != NULL || lineStartsCount == 0);
        this->_init(data, len, lineStarts, lineStartsCount);
    }

    void _init(char *data, size_t len, size_t *lineStarts, size_t lineStartsCount)
    {
        this->data = data;
        this->len = len;
        this->lineStarts = lineStarts;
        this->lineStartsCount = lineStartsCount;
    }

    ~BufferString()
    {
        if (this->data != NULL)
        {
            delete[] this->data;
            this->data = NULL;
        }
        if (this->lineStarts != NULL)
        {
            delete[] this->lineStarts;
            this->lineStarts = NULL;
        }
    }

    size_t getLen() const
    {
        return this->len;
    }

    size_t getNewLineCount() const
    {
        return this->lineStartsCount;
    }

    size_t getLastLineLen() const
    {
        if (this->lineStartsCount == 0)
        {
            return this->len;
        }
        return len - this->lineStarts[this->lineStartsCount - 1];
    }

    bool getEndsWithCR() const
    {
        return (this->len > 0 && this->data[this->len - 1] == '\r');
    }

    bool getStartsWithLF() const
    {
        return (this->len > 0 && this->data[0] == '\n');
    }

    const char *getData() const
    {
        return this->data;
    }

    const size_t *getLineStarts() const
    {
        return this->lineStarts;
    }
};

std::ostream &operator<<(std::ostream &os, BufferString *const &m)
{
    if (m == NULL)
    {
        return os << "[NULL]";
    }
    const char *data = m->getData();
    for (size_t i = 0, len = m->getLen(); i < len; i++)
    {
        os << data[i];
    }
    return os;
}

class BufferPiece
{
  private:
    BufferString *str;
    BufferPiece *leftChild;
    BufferPiece *rightChild;
    BufferPiece *parent;
    size_t len;
    size_t newLineCount;
    size_t lastLineLen;
    bool endsWithCR;
    bool startsWithLF;

  public:
    BufferPiece(BufferString *str)
    {
        assert(str != NULL);
        this->str = str;
        this->leftChild = NULL;
        this->rightChild = NULL;
        this->parent = NULL;
        this->len = str->getLen();
        this->newLineCount = str->getNewLineCount();
        this->lastLineLen = str->getLastLineLen();
        this->endsWithCR = str->getEndsWithCR();
        this->startsWithLF = str->getStartsWithLF();
    }

    BufferPiece(BufferPiece *leftChild, BufferPiece *rightChild)
    {
        assert(leftChild != NULL);
        this->str = NULL;
        this->leftChild = leftChild;
        this->rightChild = rightChild;
        this->parent = NULL;
        if (this->leftChild != NULL && this->rightChild != NULL)
        {
            if (this->leftChild->endsWithCR && this->rightChild->startsWithLF)
            {
                this->newLineCount = this->leftChild->newLineCount + this->rightChild->newLineCount - 1;
            }
            else
            {
                this->newLineCount = this->leftChild->newLineCount + this->rightChild->newLineCount;
            }
            this->len = this->leftChild->len + this->rightChild->len;
            this->lastLineLen = this->rightChild->lastLineLen;
            this->startsWithLF = this->leftChild->startsWithLF;
            this->endsWithCR = this->rightChild->endsWithCR;
        }
        else if (this->leftChild != NULL)
        {
            this->newLineCount = this->leftChild->newLineCount;
            this->len = this->leftChild->len;
            this->lastLineLen = this->leftChild->lastLineLen;
            this->startsWithLF = this->leftChild->startsWithLF;
            this->endsWithCR = this->leftChild->endsWithCR;
        }
        else
        {
            this->newLineCount = this->rightChild->newLineCount;
            this->len = this->rightChild->len;
            this->lastLineLen = this->rightChild->lastLineLen;
            this->startsWithLF = this->rightChild->startsWithLF;
            this->endsWithCR = this->rightChild->endsWithCR;
        }
    }

    ~BufferPiece()
    {
        if (this->str != NULL)
        {
            delete this->str;
            this->str = NULL;
        }
        if (this->leftChild != NULL)
        {
            delete this->leftChild;
            this->leftChild = NULL;
        }
        if (this->rightChild != NULL)
        {
            delete this->rightChild;
            this->rightChild = NULL;
        }
        this->parent = NULL;
    }

    void log()
    {
        this->log(0);
    }

    void printIndent(int indent)
    {
        for (int i = 0; i < indent; i++)
        {
            cout << " ";
        }
    }

    void log(int indent)
    {
        if (this->isLeaf())
        {
            printIndent(indent);
            cout << "leaf with " << this->newLineCount << " lines." << endl;
            return;
        }

        printIndent(indent);
        cout << "[NODE]" << endl;

        indent += 4;
        if (this->leftChild)
        {
            this->leftChild->log(indent);
        }
        if (this->rightChild)
        {
            this->rightChild->log(indent);
        }
    }

    bool isLeaf() const
    {
        return (this->str != NULL);
    }

    void setParent(BufferPiece *parent)
    {
        this->parent = parent;
    }

    size_t getLen()
    {
        return this->len;
    }

    size_t getNewLineCount()
    {
        return this->newLineCount;
    }

    BufferPiece *findPieceAtOffset(size_t &offset)
    {
        if (offset < 0)
        {
            return NULL;
        }
        if (offset >= this->len)
        {
            return NULL;
        }
        BufferPiece *it = this;
        while (!it->isLeaf())
        {
            BufferPiece *left = it->leftChild;
            size_t leftLen = left->len;
            if (offset < leftLen)
            {
                // go left
                it = left;
                continue;
            }

            // go right
            offset -= leftLen;
            it = it->rightChild;
        }

        return it;
    }

    BufferPiece *firstLeaf()
    {
        BufferPiece *res = this;
        while (res->leftChild != NULL)
        {
            res = res->leftChild;
        }
        return res;
    }

    BufferPiece *next()
    {
        assert(this->isLeaf());
        if (this->parent->leftChild == this)
        {
            BufferPiece *sibling = this->parent->rightChild;
            return sibling->firstLeaf();
        }

        BufferPiece *it = this;
        while (it->parent != NULL && it->parent->rightChild == it)
        {
            it = it->parent;
        }
        if (it->parent == NULL)
        {
            // EOF
            return NULL;
        }
        return it->parent->rightChild->firstLeaf();
    }

    SimpleString *getStrAt(const size_t _offset, const size_t _len)
    {
        if (_offset < 0 || _len < 0 || _offset + _len > this->len)
        {
            return NULL;
        }

        size_t offset = _offset;
        BufferPiece *node = this->findPieceAtOffset(offset);
        if (node == NULL)
        {
            return NULL;
        }
        return this->_getStrAt(node, offset, _len);
    }

    SimpleString *_getStrAt(BufferPiece *node, size_t offset, const size_t _len)
    {
        char *result = new char[_len];

        size_t len = _len;
        size_t resultOffset = 0;
        do
        {
            const char *src = node->str->getData();
            const size_t cnt = min(len, node->str->getLen() - offset);
            memcpy(result + resultOffset, src + offset, cnt);
            len -= cnt;
            resultOffset += cnt;
            offset = 0;

            if (len == 0)
            {
                break;
            }

            node = node->next();
            assert(node->isLeaf());
        } while (true);

        return new SimpleString(result, _len);
    }

    BufferPiece *findPieceAtLineIndex(size_t &lineIndex)
    {
        BufferPiece *it = this;
        while (!it->isLeaf())
        {
            BufferPiece *left = it->leftChild;
            BufferPiece *right = it->rightChild;

            if (left != NULL && right != NULL)
            {
                if (left->endsWithCR && right->startsWithLF)
                {
                    // one newline is split between left and right
                    // TODO
                    assert(false);
                }

                if (lineIndex <= left->newLineCount)
                {
                    // go left
                    it = left;
                    continue;
                }

                // go right
                lineIndex -= left->newLineCount;
                it = it->rightChild;
            }
        }
        return it;
    }

    size_t getLineLength(const size_t _lineNumber)
    {
        if (_lineNumber < 1 || _lineNumber > this->newLineCount + 1)
        {
            return 0;
        }

        size_t lineIndex = _lineNumber - 1;
        BufferPiece *node = findPieceAtLineIndex(lineIndex);
        if (node == NULL)
        {
            return 0;
        }

        return this->_getLineIndexLength(node, lineIndex);
    }

    size_t _getLineIndexLength(BufferPiece *node, const size_t lineIndex)
    {
        size_t nodeLineCount = node->newLineCount;
        assert(node->isLeaf());
        assert(nodeLineCount >= lineIndex);

        const size_t *lineStarts = node->str->getLineStarts();
        // for (int i = 0; i < nodeLineCount; i++)
        // {
        //     cout << i << " : " << lineStarts[i] << endl;
        // }

        const size_t lineStartOffset = (lineIndex == 0 ? 0 : lineStarts[lineIndex - 1]);

        if (lineIndex < nodeLineCount)
        {
            // lucky, the line ends in this same block
            const size_t lineEndOffset = lineStarts[lineIndex];
            return lineEndOffset - lineStartOffset;
        }

        // find the first newline or EOF
        size_t result = node->len - lineStartOffset;
        do
        {
            // TODO: could probably optimize to not visit every leaf!!!
            node = node->next();
            if (node == NULL)
            {
                // EOF
                break;
            }
            if (node->newLineCount > 0)
            {
                const size_t *lineStarts2 = node->str->getLineStarts();
                result += lineStarts2[0];
                break;
            }
            else
            {
                // node does not contain newline
                result += node->len;
            }
            // const char *src = node->str->getData();
            // const size_t cnt = min(len, node->str->getLen() - offset);
            // memcpy(result + resultOffset, src + offset, cnt);
            // len -= cnt;
            // resultOffset += cnt;
            // offset = 0;

            // if (len == 0)
            // {
            //     break;
            // }

            // node = node->next();
            // assert(node->isLeaf());
        } while (true);

        return result;
    }

    SimpleString *getLineContent(const size_t _lineNumber)
    {
        if (_lineNumber < 1 || _lineNumber > this->newLineCount + 1)
        {
            return NULL;
        }

        size_t lineIndex = _lineNumber - 1;
        BufferPiece *node = findPieceAtLineIndex(lineIndex);
        if (node == NULL)
        {
            return NULL;
        }

        const size_t *lineStarts = node->str->getLineStarts();
        // for (int i = 0; i < nodeLineCount; i++)
        // {
        //     cout << i << " : " << lineStarts[i] << endl;
        // }

        const size_t lineStartOffset = (lineIndex == 0 ? 0 : lineStarts[lineIndex - 1]);

        size_t len = this->_getLineIndexLength(node, lineIndex);
        return this->_getStrAt(node, lineStartOffset, len);
    }
};

class Buffer
{
  private:
    BufferPiece *root;

  public:
    Buffer(BufferPiece *root)
    {
        assert(root != NULL);
        this->root = root;
    }

    ~Buffer()
    {
        delete this->root;
    }

    size_t getLen() const
    {
        return this->root->getLen();
    }

    size_t getLineCount() const
    {
        return this->root->getNewLineCount() + 1;
    }

    SimpleString *getStrAt(size_t offset, size_t len)
    {
        return this->root->getStrAt(offset, len);
    }

    size_t getLineLength(size_t lineNumber)
    {
        return this->root->getLineLength(lineNumber);
    }

    SimpleString *getLineContent(size_t lineNumber)
    {
        return this->root->getLineContent(lineNumber);
    }
};

BufferPiece *buildBufferFromPieces(vector<BufferString *> &pieces, size_t start, size_t end)
{
    size_t cnt = end - start;

    if (cnt == 0)
    {
        return NULL;
    }

    if (cnt == 1)
    {
        return new BufferPiece(pieces[start]);
    }

    size_t mid = (start + cnt / 2);

    BufferPiece *left = buildBufferFromPieces(pieces, start, mid);
    BufferPiece *right = buildBufferFromPieces(pieces, mid, end);

    BufferPiece *result = new BufferPiece(left, right);
    left->setParent(result);
    right->setParent(result);

    return result;
}

Buffer *buildBufferFromFile(const char *filename)
{
    ifstream ifs(filename, ifstream::binary);
    if (!ifs)
    {
        return NULL;
    }
    vector<BufferString *> vPieces;
    ifs.seekg(0, std::ios::beg);
    while (!ifs.eof())
    {
        char *piece = new char[PIECE_SIZE];

        ifs.read(piece, PIECE_SIZE);

        if (ifs)
        {
            vPieces.push_back(new BufferString(piece, PIECE_SIZE));
        }
        else
        {
            vPieces.push_back(new BufferString(piece, ifs.gcount()));
        }
    }
    ifs.close();

    BufferPiece *root = buildBufferFromPieces(vPieces, 0, vPieces.size());
    // root->log();
    return new Buffer(root);
}

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
