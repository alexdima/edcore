
#include "edcore.cpp"

// typedef void (*TestCase)();
// vector<TestCase> testCases;

// // #define TEST_CASE(fn) (void fn(); testCases.push_back(fn); void fn())

// // void (*foo)();

// // void t1(); testCases.push_back(t1);
// // void t1()
// // {

// // }

// #define TEST_ADD(fp) testCases.push_back(fp);

// class TestSuite
// {
// protected:
//   vector<TestCase> testCases;

// };

// class CheckerTestSuite : public TestSuite
// {
// public:
//   CheckerTestSuite()
//   {
//       TEST_ADD(CheckerTestSuite::t1);
//   }

//   void t1()
//   {

//   }
// };

// TEST_CASE(f1) {

// }

Buffer *checker()
{
    return buildBufferFromFile("tests/checker.ts");
}

int compareFiles(FILE *file_compared, FILE *file_checked)
{
}

int compareFiles(const char *filename1, const char *filename2)
{
    FILE *f1 = fopen(filename1, "rb");
    FILE *f2 = fopen(filename2, "rb");

    bool diff = 0;
    int N = 65536;
    char *b1 = (char *)calloc(1, N + 1);
    char *b2 = (char *)calloc(1, N + 1);
    size_t s1, s2;

    do
    {
        s1 = fread(b1, 1, N, f1);
        s2 = fread(b2, 1, N, f2);

        if (s1 != s2 || memcmp(b1, b2, s1))
        {
            diff = 1;
            break;
        }
    } while (!feof(f1) || !feof(f2));

    free(b1);
    free(b2);

    fclose(f1);
    fclose(f2);

    if (diff)
        return 0;
    else
        return 1;
}

void check(const char *testName, const char *actualFileName, const char *expectedFileName)
{
    int r = compareFiles(actualFileName, expectedFileName);

    if (r == 1)
    {
        cout << "[OK] " << testName << endl;
    }
    else
    {
        cout << "[FAIL] " << testName << endl;
    }
}

// Get line one by one
void t1()
{
    Buffer *buffer = checker();
    ofstream f("tests/t1.actual", ofstream::binary);

    size_t lineCount = buffer->getLineCount();
    for (size_t lineNumber = 1; lineNumber <= lineCount; lineNumber++)
    {
        SimpleString *str = buffer->getLineContent(lineNumber);
        f << str;
        delete str;
    }

    f.close();
    delete buffer;
    check("t1", "tests/t1.actual", "tests/t1.expected");
}

void t2()
{
    Buffer *buffer = checker();
    ofstream f("tests/t2.actual", ofstream::binary);

    size_t lineCount = buffer->getLineCount();
    for (size_t lineNumber = 1; lineNumber <= lineCount; lineNumber++)
    {
        f << buffer->getLineLength(lineNumber) << endl;
    }

    f.close();
    delete buffer;
    check("t2", "tests/t2.actual", "tests/t2.expected");
}

int main(void)
{
    t1();
    t2();
}
