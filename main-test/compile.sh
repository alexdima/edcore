g++ -g "main.cpp" \
        "../src/core/array.h" \
        "../src/core/buffer-string.cc" \
        "../src/core/buffer-string.h" \
        "../src/core/buffer-piece.cc" \
        "../src/core/buffer-piece.h" \
        "../src/core/buffer.cc" \
        "../src/core/buffer.h" \
        "../src/core/buffer-builder.cc"


# ./compile.sh && valgrind --leak-check=full --show-leak-kinds=all ./a.out 2>leaks.txt
