// /*---------------------------------------------------------------------------------------------
//  *  Copyright (c) Microsoft Corporation. All rights reserved.
//  *  Licensed under the MIT License. See License.txt in the project root for license information.
//  *--------------------------------------------------------------------------------------------*/

// #define TRACK_MEMORY

// #include "edcore.cpp"

// // typedef void (*TestCase)();
// // vector<TestCase> testCases;

// // // #define TEST_CASE(fn) (void fn(); testCases.push_back(fn); void fn())

// // // void (*foo)();

// // // void t1(); testCases.push_back(t1);
// // // void t1()
// // // {

// // // }

// // #define TEST_ADD(fp) testCases.push_back(fp);

// // class TestSuite
// // {
// // protected:
// //   vector<TestCase> testCases;

// // };

// // class CheckerTestSuite : public TestSuite
// // {
// // public:
// //   CheckerTestSuite()
// //   {
// //       TEST_ADD(CheckerTestSuite::t1);
// //   }

// //   void t1()
// //   {

// //   }
// // };

// // TEST_CASE(f1) {

// // }

// Buffer *checker()
// {
//     return buildBufferFromFile("tests/checker.ts");
// }

// int compareFiles(const char *filename1, const char *filename2)
// {
//     FILE *f1 = fopen(filename1, "rb");
//     FILE *f2 = fopen(filename2, "rb");

//     bool diff = 0;
//     int N = 65536;
//     char *b1 = (char *)calloc(1, N + 1);
//     char *b2 = (char *)calloc(1, N + 1);
//     size_t s1, s2;

//     do
//     {
//         s1 = fread(b1, 1, N, f1);
//         s2 = fread(b2, 1, N, f2);

//         if (s1 != s2 || memcmp(b1, b2, s1))
//         {
//             diff = 1;
//             break;
//         }
//     } while (!feof(f1) || !feof(f2));

//     free(b1);
//     free(b2);

//     fclose(f1);
//     fclose(f2);

//     if (diff)
//         return 0;
//     else
//         return 1;
// }

// void check(const char *testName, const char *actualFileName, const char *expectedFileName)
// {
//     int r = compareFiles(actualFileName, expectedFileName);

//     if (r == 1)
//     {
//         cout << "[OK] " << testName << endl;
//     }
//     else
//     {
//         cout << "[FAIL] " << testName << endl;
//     }
// }

// // Get line one by one
// void t1()
// {
//     Buffer *buffer = checker();
//     ofstream f("tests/t1.actual", ofstream::binary);

//     MM_DUMP(f);

//     size_t lineCount = buffer->getLineCount();
//     for (size_t lineNumber = 1; lineNumber <= lineCount; lineNumber++)
//     {
//         shared_ptr<String> str = buffer->getLineContent(lineNumber);
//         f << str;
//     }

//     TIME_START(t1);
//     for (size_t lineNumber = 1; lineNumber <= lineCount; lineNumber++)
//     {
//         shared_ptr<String> str = buffer->getLineContent(lineNumber);
//     }
//     TIME_END(cout, t1, "t1 - getLineContent multiple times");

//     MM_DUMP(f);

//     delete buffer;
//     MM_DUMP(f);

//     f.close();
//     check("t1", "tests/t1.actual", "tests/t1.expected");
// }

// void t2()
// {
//     Buffer *buffer = checker();
//     ofstream f("tests/t2.actual", ofstream::binary);

//     MM_DUMP(f);

//     size_t lineCount = buffer->getLineCount();
//     for (size_t lineNumber = 1; lineNumber <= lineCount; lineNumber++)
//     {
//         f << buffer->getLineLength(lineNumber) << endl;
//     }

//     delete buffer;
//     MM_DUMP(f);

//     f.close();
//     check("t2", "tests/t2.actual", "tests/t2.expected");
// }

// void t3()
// {
//     Buffer *buffer = checker();
//     ofstream f("tests/t3.actual", ofstream::binary);
//     MM_DUMP(f);

//     f << buffer;

//     delete buffer;
//     MM_DUMP(f);

//     f.close();
//     check("t3", "tests/t3.actual", "tests/t3.expected");
// }

// void t4()
// {
//     Buffer *buffer = checker();
//     ofstream f("tests/t4.actual", ofstream::binary);

//     MM_DUMP(f);

//     vector<shared_ptr<String>> lines;
//     size_t lineCount = buffer->getLineCount();
//     for (size_t lineNumber = 1; lineNumber <= lineCount; lineNumber++)
//     {
//         lines.push_back(buffer->getLineContent(lineNumber));
//     }

//     delete buffer;
//     MM_DUMP(f);

//     for (size_t i = 0; i < lines.size(); i++)
//     {
//         f << lines[i];
//     }

//     lines.clear();
//     MM_DUMP(f);

//     f.close();
//     check("t4", "tests/t4.actual", "tests/t4.expected");
// }

// int main(void)
// {
//     t1();
//     t2();
//     t3();
//     t4();
// }
