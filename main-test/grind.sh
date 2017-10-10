./compile.sh && valgrind --leak-check=full --show-leak-kinds=all ./a.out 2>leaks.txt
