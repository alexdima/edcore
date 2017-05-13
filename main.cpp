
#include "edcore.cpp"

int main(void)
{
    //    cout << "Hello world!" << endl;

    Buffer *buffer = buildBufferFromFile("test.txt");
    cout << "buffer->getLineCount(): " << buffer->getLineCount() << endl;
    cout << "buffer->getLen(): " << buffer->getLen() << endl;

    SimpleString *str = buffer->getStrAt(1361286, 16);
    cout << "buffer->getStrAt(1361286, 16): " << str << endl;
    if (str != NULL)
    {
        delete str;
    }

#define BILLION 1E9

    timespec time1, time2;

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time1);
    // struct timespec requestStart, requestEnd;
    // clock_gettime(CLOCK_REALTIME, &requestStart);
    for (int i = 1; i <= buffer->getLineCount(); i++)
    {
        int len = buffer->getLineLength(i);
        // SimpleString *line = buffer->getLineContent(i);
        // delete line;
        // cout << len << " ";
    }
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &time2);
    timespec took = diff(time1, time2);
    double ms = took.tv_sec * 1000 + took.tv_nsec / (double)1000000;
    cout << "TOOK " << ms << "ms." << endl;
    // cout<<diff(time1,time2).tv_sec<<":"<<diff(time1,time2).tv_nsec<<endl;
    // clock_gettime(CLOCK_REALTIME, &requestEnd);

    //     if (requestEnd.tv_sec == requestStart.tv_sec)
    //     {

    //     }
    // // Calculate time it took
    // double accum = ( requestEnd.tv_sec - requestStart.tv_sec )
    //   + ( requestEnd.tv_nsec - requestStart.tv_nsec )
    //   / BILLION;
    // printf( "%lf seconds\n", accum );
    // cout << endl;

    // str = buffer->getStrAt(0, buffer->getLen());
    // cout << "buffer->getStrAt(0, 4 * 1024 + 4 * 1024 + 1): " << str << endl;
    // if (str != NULL)
    // {
    //     delete str;
    // }

    cout << "buffer->getLineLength(24427): " << buffer->getLineLength(24427) << endl;

    SimpleString *line = buffer->getLineContent(24427);
    cout << "buffer->getLineContent(24427): " << line << endl;
    if (line != NULL)
    {
        delete line;
    }

    delete buffer;
    return 0;
}
