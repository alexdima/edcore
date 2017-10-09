
#include <stdio.h>
#include <string>
#include <fstream>
#include <streambuf>

#include "../src/core/buffer.h"
#include "../src/core/buffer-builder.h"

#define FIXTURE "../test/fixtures/checker-10.txt"
#define CHUNK_SIZE 12435

edcore::Buffer *buildBufferFromFixture(const char *filename);

class SimpleString : public edcore::BufferString
{
  public:
    SimpleString(const char* data, size_t len) { data_=data; len_ = len; }
    size_t length() const { return len_; }
    void write(uint16_t *buffer, size_t start, size_t length) const {
        for (size_t i = 0; i < length; i++)
        {
            buffer[i] = data_[i + start];
        }
    }
    void writeOneByte(uint8_t *buffer, size_t start, size_t length) const {
        for (size_t i = 0; i < length; i++)
        {
            buffer[i] = data_[i + start];
        }
    }
    bool isOneByte() const { return true; }
    bool containsOnlyOneByte() const { return true; }

  private:
    const char* data_;
    size_t len_;
};

int main(void) {

    printf("Hello world!");

    edcore::Buffer *buff = buildBufferFromFixture(NULL);

    vector<edcore::OffsetLenEdit2> edits(1);

    edits[0].initialIndex = 0;
    edits[0].offset = 189;
    edits[0].length = 11;
    edits[0].text = new SimpleString("eesfuopnzzphpjzyghusg\r\nixflgeryrn\r\natcvkjdvcmqwqxipkvhtiuldxztdrbuhygwmiqtaoyzymzantqkdbgg\nfxhvkf\ro\nxfatpi\rwomfrh", 113);

    // void replaceOffsetLen(vector<OffsetLenEdit2> &edits);


    buff->replaceOffsetLen(edits);

    delete edits[0].text;
    edits.clear();

    delete buff;
    buff = NULL;

    // offset: 189,
    // length: 11,
    // text: 'eesfuopnzzphpjzyghusg\r\nixflgeryrn\r\natcvkjdvcmqwqxipkvhtiuldxztdrbuhygwmiqtaoyzymzantqkdbgg\nfxhvkf\ro\nxfatpi\rwomfrh'


    return 0;
}

edcore::Buffer *buildBufferFromFixture(const char *filename) {

    edcore::BufferBuilder *builder = new edcore::BufferBuilder();
    char *buff = new char[CHUNK_SIZE];
    uint16_t *buff2 = new uint16_t[CHUNK_SIZE];

    FILE *f = fopen(FIXTURE, "rt");
    if (f == NULL)
    {
        printf("CANNOT OPEN FILE\n");
        return NULL;
    }
    while (!feof(f))
    {
        size_t read = fread(buff, 1, CHUNK_SIZE, f);
        for (size_t i = 0; i < read; i++)
        {
            buff2[i] = buff[i];
        }
        builder->AcceptChunk(buff2, read);
    }
    fclose(f);

    delete []buff;
    delete []buff2;

    builder->Finish();

    edcore::Buffer *result = builder->Build();
    delete builder;
    return result;

    // std::ifstream t("../test/fixutres/checker-10.txt");
    // std::string str;

    // t.seekg(0, std::ios::end);
    // str.reserve(t.tellg());
    // t.seekg(0, std::ios::beg);

    // str.assign((std::istreambuf_iterator<char>(t)),
    //             std::istreambuf_iterator<char>());

}
