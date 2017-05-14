#include <iostream>
#include <memory>

#include <fstream>
#include <vector>
#include <cstring>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>

using namespace std;

#define PIECE_SIZE 4 * 1024 // 4KB

class SimpleString;
class BufferString;
class BufferSubstring;
class BufferNode;
class Buffer;

#define TRACK_MEMORY

#ifdef TRACK_MEMORY

#define MM_REGISTER(what) this->_mm = mm; this->_mm->_register(this)
#define MM_UNREGISTER(what) this->_mm->_unregister(this)

#else

#define MM_REGISTER(what) this->_mm = NULL
#define MM_UNREGISTER(what)

#endif

class MemManager
{
  private:
    size_t _simpleStringCnt;
    size_t _bufferStringCnt;
    size_t _bufferSubstringCnt;
    size_t _bufferNodeCnt;
    size_t _bufferCnt;

  public:
    MemManager()
    {
        this->_simpleStringCnt = 0;
        this->_bufferStringCnt = 0;
        this->_bufferSubstringCnt = 0;
        this->_bufferNodeCnt = 0;
        this->_bufferCnt = 0;
    }

    void _register(SimpleString *simpleString)
    {
        this->_simpleStringCnt++;
    }

    void _unregister(SimpleString *simpleString)
    {
        this->_simpleStringCnt--;
    }

    void _register(BufferString *bufferString)
    {
        this->_bufferStringCnt++;
    }

    void _unregister(BufferString *bufferString)
    {
        this->_bufferStringCnt--;
    }

    void _register(BufferSubstring *bufferSubstring)
    {
        this->_bufferSubstringCnt++;
    }

    void _unregister(BufferSubstring *bufferSubstring)
    {
        this->_bufferSubstringCnt--;
    }

    void _register(BufferNode *bufferNode)
    {
        this->_bufferNodeCnt++;
    }

    void _unregister(BufferNode *bufferNode)
    {
        this->_bufferNodeCnt--;
    }

    void _register(Buffer *buffer)
    {
        this->_bufferCnt++;
    }

    void _unregister(Buffer *buffer)
    {
        this->_bufferCnt--;
    }

    void print(ostream &os) const
    {
        os << "MM [" << endl;
        os << "  -> simpleStringCnt: " << this->_simpleStringCnt << endl;
        os << "  -> bufferStringCnt: " << this->_bufferStringCnt << endl;
        os << "  -> bufferSubstringCnt: " << this->_bufferSubstringCnt << endl;
        os << "  -> bufferNodeCnt: " << this->_bufferNodeCnt << endl;
        os << "  -> bufferCnt: " << this->_bufferCnt << endl;
        os << "]" << endl;
    }
};
std::ostream &operator<<(std::ostream &os, MemManager *const &m)
{
    if (m == NULL)
    {
        return os << "[NULL]";
    }

    m->print(os);
    return os;
}

class String
{
  public:
    virtual void print(std::ostream &os) = 0;
};

std::ostream &operator<<(std::ostream &os, String *const &m)
{
    if (m == NULL)
    {
        return os << "[NULL]";
    }

    m->print(os);
    return os;
}
std::ostream &operator<<(std::ostream &os, shared_ptr<String> const &m)
{
    if (m == NULL)
    {
        return os << "[NULL]";
    }

    m->print(os);
    return os;
}

class SimpleString : public String
{
  private:
    MemManager *_mm;
    char *_data;
    size_t _len;

  public:
    SimpleString(MemManager *mm, char *data, size_t len)
    {
        this->_data = data;
        this->_len = len;
        MM_REGISTER(SimpleString);
    }

    ~SimpleString()
    {
        MM_UNREGISTER(this);
        if (this->_data != NULL)
        {
            delete[] this->_data;
            this->_data = NULL;
        }
    }

    void print(std::ostream &os)
    {
        const char *data = this->_data;
        const size_t len = this->_len;
        for (size_t i = 0; i < len; i++)
        {
            os << data[i];
        }
    }
};

void printIndent(ostream &os, int indent)
{
    for (int i = 0; i < indent; i++)
    {
        os << " ";
    }
}

class BufferString : public String
{
  private:
    MemManager *_mm;
    char *_data;
    size_t _len;

    size_t *_lineStarts;
    size_t _lineStartsCount;

    void _init(MemManager *mm, char *data, size_t len, size_t *lineStarts, size_t lineStartsCount)
    {
        this->_data = data;
        this->_len = len;
        this->_lineStarts = lineStarts;
        this->_lineStartsCount = lineStartsCount;
        MM_REGISTER(this);
    }

  public:
    BufferString(MemManager *mm, char *data, size_t len)
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
        this->_init(mm, data, len, lineStarts, lineStartsCount);
    }

    ~BufferString()
    {
        MM_UNREGISTER(this);
        if (this->_data != NULL)
        {
            delete[] this->_data;
            this->_data = NULL;
        }
        if (this->_lineStarts != NULL)
        {
            delete[] this->_lineStarts;
            this->_lineStarts = NULL;
        }
    }

    size_t getLen() const
    {
        return this->_len;
    }

    size_t getNewLineCount() const
    {
        return this->_lineStartsCount;
    }

    bool getEndsWithCR() const
    {
        return (this->_len > 0 && this->_data[this->_len - 1] == '\r');
    }

    bool getStartsWithLF() const
    {
        return (this->_len > 0 && this->_data[0] == '\n');
    }

    const char *getData() const // TODO
    {
        return this->_data;
    }

    const size_t *getLineStarts() const
    {
        return this->_lineStarts;
    }

    void print(std::ostream &os)
    {
        const char *data = this->_data;
        const size_t len = this->_len;
        for (size_t i = 0; i < len; i++)
        {
            os << data[i];
        }
    }
};

class BufferStringLinkedListNode
{
  private:
    shared_ptr<BufferString> _str;
    BufferStringLinkedListNode *_next;

  public:
    BufferStringLinkedListNode(MemManager *mm, shared_ptr<BufferString> str)
    {
        this->_str = str;
        this->_next = NULL;
    }

    ~BufferStringLinkedListNode()
    {
        this->_next = NULL;
    }

    shared_ptr<BufferString> getStr() const
    {
        return this->_str;
    }

    void setNext(BufferStringLinkedListNode *next)
    {
        this->_next = next;
    }

    BufferStringLinkedListNode *getNext() const
    {
        return this->_next;
    }
};

class BufferStringLinkedList
{
  private:
    BufferStringLinkedListNode *_first;
    BufferStringLinkedListNode *_last;

  public:
    BufferStringLinkedList(MemManager *mm)
    {
        this->_first = NULL;
        this->_last = NULL;
    }

    ~BufferStringLinkedList()
    {
        BufferStringLinkedListNode *node = this->_first;
        while (node != NULL)
        {
            BufferStringLinkedListNode *next = node->getNext();
            delete node;
            node = next;
        }
        this->_first = NULL;
        this->_last = NULL;
    }

    void append(BufferStringLinkedListNode *node)
    {
        if (this->_first == NULL)
        {
            this->_first = node;
            this->_last = node;
            return;
        }

        this->_last->setNext(node);
        this->_last = node;
    }

    BufferStringLinkedListNode *getFirst() const
    {
        return this->_first;
    }
};

class BufferSubstring : public String
{
  private:
    MemManager *_mm;
    BufferStringLinkedList *_bufferStringLinkedList;
    size_t _startOffset;
    size_t _len;

  public:
    BufferSubstring(MemManager *mm, BufferStringLinkedList *bufferStringLinkedList, size_t startOffset, size_t len)
    {
        this->_bufferStringLinkedList = bufferStringLinkedList;
        this->_startOffset = startOffset;
        this->_len = len;
        MM_REGISTER(this);
    }

    ~BufferSubstring()
    {
        MM_UNREGISTER(this);
        if (this->_bufferStringLinkedList)
        {
            delete this->_bufferStringLinkedList;
            this->_bufferStringLinkedList = NULL;
        }
    }

    void print(std::ostream &os)
    {
        // char *result = new char[len];

        // size_t resultOffset = 0;
        // size_t remainingLen = len;
        // do
        // {
        //     const char *src = node->_str->getData();
        //     const size_t cnt = min(remainingLen, node->_str->getLen() - offset);
        //     memcpy(result + resultOffset, src + offset, cnt);
        //     remainingLen -= cnt;
        //     resultOffset += cnt;
        //     offset = 0;

        //     if (remainingLen == 0)
        //     {
        //         break;
        //     }

        //     node = node->next();
        //     assert(node->isLeaf());
        // } while (true);

        // return shared_ptr<SimpleString>(new SimpleString(this->_mm, result, len));

        BufferStringLinkedListNode *node = this->_bufferStringLinkedList->getFirst();
        size_t remainingLen = this->_len;
        size_t offset = this->_startOffset;
        do
        {
            shared_ptr<BufferString> str = node->getStr();
            const char *src = str->getData();
            const size_t cnt = min(remainingLen, str->getLen() - offset);

            for (size_t i = 0; i < cnt; i++)
            {
                os << src[i + offset];
            }

            remainingLen -= cnt;
            offset = 0;

            if (remainingLen == 0)
            {
                break;
            }
            node = node->getNext();

        } while (true);
    }
};

class BufferNode
{
  private:
    MemManager *_mm;
    shared_ptr<BufferString> _str;

    BufferNode *_leftChild;
    BufferNode *_rightChild;

    BufferNode *_parent;
    size_t _len;
    size_t _newLineCount;
    bool _endsWithCR;
    bool _startsWithLF;

    void _init(
        MemManager *mm,
        shared_ptr<BufferString> str,
        BufferNode *leftChild,
        BufferNode *rightChild,
        size_t len,
        size_t newLineCount,
        bool startsWithLF,
        bool endsWithCR)
    {
        this->_str = str;
        this->_leftChild = leftChild;
        this->_rightChild = rightChild;
        this->_parent = NULL;
        this->_len = len;
        this->_newLineCount = newLineCount;
        this->_startsWithLF = startsWithLF;
        this->_endsWithCR = endsWithCR;
        MM_REGISTER(this);
    }

  public:
    BufferNode(MemManager *mm, shared_ptr<BufferString> str)
    {
        assert(str != NULL);
        this->_init(mm, str, NULL, NULL, str->getLen(), str->getNewLineCount(), str->getStartsWithLF(), str->getEndsWithCR());
    }

    BufferNode(MemManager *mm, BufferNode *leftChild, BufferNode *rightChild)
    {
        assert(leftChild != NULL || rightChild != NULL);

        const size_t len = (leftChild != NULL ? leftChild->_len : 0) + (rightChild != NULL ? rightChild->_len : 0);
        const size_t newLineCount = (leftChild != NULL ? leftChild->_newLineCount : 0) + (rightChild != NULL ? rightChild->_newLineCount : 0);
        const size_t discountNewLine = (leftChild != NULL && rightChild != NULL && leftChild->_endsWithCR && rightChild->_startsWithLF);
        const bool startsWithLF = (leftChild != NULL ? leftChild->_startsWithLF : rightChild->_startsWithLF);
        const bool endsWithCR = (rightChild != NULL ? rightChild->_endsWithCR : leftChild->_endsWithCR);

        this->_init(mm, NULL, leftChild, rightChild, len, newLineCount - (discountNewLine ? 1 : 0), startsWithLF, endsWithCR);
    }

    ~BufferNode()
    {
        MM_UNREGISTER(this);
        // if (this->_str != NULL)
        // {
        //     delete this->_str;
        //     this->_str = NULL;
        // }
        if (this->_leftChild != NULL)
        {
            delete this->_leftChild;
            this->_leftChild = NULL;
        }
        if (this->_rightChild != NULL)
        {
            delete this->_rightChild;
            this->_rightChild = NULL;
        }
        this->_parent = NULL;
    }

    void print(ostream &os)
    {
        this->log(os, 0);
    }

    void log(ostream &os, int indent)
    {
        if (this->isLeaf())
        {
            printIndent(os, indent);
            os << "[LEAF] (len:" << this->_len << ", newLineCount:" << this->_newLineCount;
            if (this->_startsWithLF)
            {
                os << ", startsWithLF";
            }
            if (this->_endsWithCR)
            {
                os << ", endsWithCR";
            }
            os << ")" << endl;
            return;
        }

        printIndent(os, indent);
        os << "[NODE] (len:" << this->_len << ", newLineCount:" << this->_newLineCount;
        if (this->_startsWithLF)
        {
            os << ", startsWithLF";
        }
        if (this->_endsWithCR)
        {
            os << ", endsWithCR";
        }
        os << ")" << endl;

        indent += 4;
        if (this->_leftChild)
        {
            this->_leftChild->log(os, indent);
        }
        if (this->_rightChild)
        {
            this->_rightChild->log(os, indent);
        }
    }

    bool isLeaf() const
    {
        return (this->_str != NULL);
    }

    void setParent(BufferNode *parent)
    {
        this->_parent = parent;
    }

    size_t getLen() const
    {
        return this->_len;
    }

    size_t getNewLineCount() const
    {
        return this->_newLineCount;
    }

    BufferNode *findPieceAtOffset(size_t &offset)
    {
        if (offset >= this->_len)
        {
            return NULL;
        }

        BufferNode *it = this;
        while (!it->isLeaf())
        {
            BufferNode *left = it->_leftChild;
            BufferNode *right = it->_rightChild;

            if (left == NULL)
            {
                it = right;
                continue;
            }

            if (right == NULL)
            {
                it = left;
                continue;
            }

            const size_t leftLen = left->_len;
            if (offset < leftLen)
            {
                // go left
                it = left;
                continue;
            }

            // go right
            offset -= leftLen;
            it = right;
        }

        return it;
    }

    BufferNode *firstLeaf()
    {
        // TODO: this will not work for an unbalanced tree
        BufferNode *res = this;
        while (res->_leftChild != NULL)
        {
            res = res->_leftChild;
        }
        return res;
    }

    BufferNode *next()
    {
        assert(this->isLeaf());
        if (this->_parent->_leftChild == this)
        {
            BufferNode *sibling = this->_parent->_rightChild;
            return sibling->firstLeaf();
        }

        BufferNode *it = this;
        while (it->_parent != NULL && it->_parent->_rightChild == it)
        {
            it = it->_parent;
        }
        if (it->_parent == NULL)
        {
            // EOF
            return NULL;
        }
        return it->_parent->_rightChild->firstLeaf();
    }

    shared_ptr<String> getStrAt(size_t offset, size_t len)
    {
        if (offset + len > this->_len)
        {
            return NULL;
        }

        BufferNode *node = this->findPieceAtOffset(offset);
        if (node == NULL)
        {
            return NULL;
        }

        return this->_getStrAt(node, offset, len);
    }

    shared_ptr<String> _getStrAt(BufferNode *node, size_t offset, size_t len)
    {
        BufferStringLinkedList *snapshot = new BufferStringLinkedList(this->_mm);

                // char *result = new char[len];

        // size_t resultOffset = 0;
        size_t remainingLen = len;
        size_t startOffset = offset;
        do
        {
            shared_ptr<BufferString> str = node->_str;
            const char *src = node->_str->getData();
            const size_t cnt = min(remainingLen, node->_str->getLen() - startOffset);
        //     memcpy(result + resultOffset, src + offset, cnt);
            remainingLen -= cnt;
        //     resultOffset += cnt;
            startOffset = 0;

            BufferStringLinkedListNode *node2 = new BufferStringLinkedListNode(this->_mm, str);
            snapshot->append(node2);

            if (remainingLen == 0)
            {
                break;
            }

            node = node->next();
        //     assert(node->isLeaf());
        } while (true);


        return shared_ptr<BufferSubstring>(new BufferSubstring(this->_mm, snapshot, offset, len));
    }

    BufferNode *findPieceAtLineIndex(size_t &lineIndex)
    {
        BufferNode *it = this;
        while (!it->isLeaf())
        {
            BufferNode *left = it->_leftChild;
            BufferNode *right = it->_rightChild;

            if (left == NULL)
            {
                // go right
                it = right;
            }

            if (right == NULL)
            {
                // go left
                it = left;
            }

            if (left->_endsWithCR && right->_startsWithLF)
            {
                // one newline is split between left and right
                // TODO
                assert(false);
            }

            if (lineIndex <= left->_newLineCount)
            {
                // go left
                it = left;
                continue;
            }

            // go right
            lineIndex -= left->_newLineCount;
            it = right;
        }

        return it;
    }

    size_t getLineLength(size_t lineNumber)
    {
        if (lineNumber < 1 || lineNumber > this->_newLineCount + 1)
        {
            return 0;
        }

        size_t lineIndex = lineNumber - 1;
        BufferNode *node = findPieceAtLineIndex(lineIndex);
        if (node == NULL)
        {
            return 0;
        }

        const size_t *lineStarts = node->_str->getLineStarts();
        const size_t lineStartOffset = (lineIndex == 0 ? 0 : lineStarts[lineIndex - 1]);

        return this->_getLineIndexLength(lineIndex, node, lineStartOffset);
    }

    size_t _getLineIndexLength(const size_t lineIndex, BufferNode *node, const size_t lineStartOffset)
    {
        const size_t nodeLineCount = node->_newLineCount;
        assert(node->isLeaf());
        assert(nodeLineCount >= lineIndex);

        if (lineIndex < nodeLineCount)
        {
            // lucky, the line ends in this same block
            const size_t *lineStarts = node->_str->getLineStarts();
            const size_t lineEndOffset = lineStarts[lineIndex];
            return lineEndOffset - lineStartOffset;
        }

        // find the first newline or EOF
        size_t result = node->_len - lineStartOffset;
        do
        {
            // TODO: could probably optimize to not visit every leaf!!!
            node = node->next();

            if (node == NULL)
            {
                // EOF
                break;
            }

            if (node->_newLineCount > 0)
            {
                const size_t *lineStarts = node->_str->getLineStarts();
                result += lineStarts[0];
                break;
            }

            // node does not contain newline
            result += node->_len;

        } while (true);

        return result;
    }

    shared_ptr<String> getLineContent(size_t lineNumber)
    {
        if (lineNumber < 1 || lineNumber > this->_newLineCount + 1)
        {
            return NULL;
        }

        size_t lineIndex = lineNumber - 1;
        BufferNode *node = findPieceAtLineIndex(lineIndex);
        if (node == NULL)
        {
            return NULL;
        }

        const size_t *lineStarts = node->_str->getLineStarts();
        const size_t lineStartOffset = (lineIndex == 0 ? 0 : lineStarts[lineIndex - 1]);
        size_t len = this->_getLineIndexLength(lineIndex, node, lineStartOffset);
        return this->_getStrAt(node, lineStartOffset, len);
    }
};

class Buffer
{
  private:
    MemManager *_mm;
    BufferNode *root;

  public:
    Buffer(MemManager *mm, BufferNode *root)
    {
        assert(root != NULL);
        this->root = root;
        MM_REGISTER(this);
    }

    ~Buffer()
    {
        MM_UNREGISTER(this);
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

    shared_ptr<String> getStrAt(size_t offset, size_t len)
    {
        return this->root->getStrAt(offset, len);
    }

    size_t getLineLength(size_t lineNumber)
    {
        return this->root->getLineLength(lineNumber);
    }

    shared_ptr<String> getLineContent(size_t lineNumber)
    {
        return this->root->getLineContent(lineNumber);
    }

    void print(ostream &os)
    {
        this->root->print(os);
    }
};
std::ostream &operator<<(std::ostream &os, Buffer *const &m)
{
    if (m == NULL)
    {
        return os << "[NULL]";
    }

    m->print(os);
    return os;
}

BufferNode *buildBufferFromPieces(MemManager *mm, vector<shared_ptr<BufferString>> &pieces, size_t start, size_t end)
{
    size_t cnt = end - start;

    if (cnt == 0)
    {
        return NULL;
    }

    if (cnt == 1)
    {
        return new BufferNode(mm, pieces[start]);
    }

    size_t mid = (start + cnt / 2);

    BufferNode *left = buildBufferFromPieces(mm, pieces, start, mid);
    BufferNode *right = buildBufferFromPieces(mm, pieces, mid, end);

    BufferNode *result = new BufferNode(mm, left, right);
    left->setParent(result);
    right->setParent(result);

    return result;
}

// class _BufferString
// {
//   public:
//     char *data;
//     size_t len;

//     _BufferString(char *data, size_t len)
//     {
//         this->data = data;
//         this->len = len;
//     }
// };

Buffer *buildBufferFromFile(MemManager *mm, const char *filename)
{
    ifstream ifs(filename, ifstream::binary);
    if (!ifs)
    {
        return NULL;
    }
    vector<shared_ptr<BufferString>> rawPieces;
    ifs.seekg(0, std::ios::beg);
    while (!ifs.eof())
    {
        char *piece = new char[PIECE_SIZE];

        ifs.read(piece, PIECE_SIZE);

        if (ifs)
        {
            rawPieces.push_back(shared_ptr<BufferString>(new BufferString(mm, piece, PIECE_SIZE)));
        }
        else
        {
            rawPieces.push_back(shared_ptr<BufferString>(new BufferString(mm, piece, ifs.gcount())));
        }
    }
    ifs.close();

    size_t pieceCount = rawPieces.size();

    // Build BufferString
    // shared_ptr<BufferString> *vPieces = new shared_ptr<BufferString>[ pieceCount ];
    // shared_ptr<BufferString> next = NULL;
    // for (int i = pieceCount - 1; i >= 0; i--)
    // {
    //     shared_ptr<_BufferString> src = rawPieces[i];
    //     next = shared_ptr<BufferString>(new BufferString(mm, next, src->data, src->len));
    //     vPieces[i] = next;
    // }

    BufferNode *root = buildBufferFromPieces(mm, rawPieces, 0, pieceCount);
    // delete[] vPieces;

    // root->log();
    return new Buffer(mm, root);
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
