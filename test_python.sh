#! /bin/bash

python ./python/huffman.py encode ./test_cases/cn.txt ./test_out/cn.huff
python ./python/huffman.py encode ./test_cases/empty.txt ./test_out/empty.huff
python ./python/huffman.py encode ./test_cases/graph.txt ./test_out/graph.huff
python ./python/huffman.py encode ./test_cases/text.txt ./test_out/text.huff
python ./python/huffman.py encode ./test_cases/unique.txt ./test_out/unique.huff

python ./python/huffman.py decode ./test_out/cn.huff ./test_out/cn.huff.txt
python ./python/huffman.py decode ./test_out/empty.huff ./test_out/empty.huff.txt
python ./python/huffman.py decode ./test_out/graph.huff ./test_out/graph.huff.txt
python ./python/huffman.py decode ./test_out/text.huff ./test_out/text.huff.txt
python ./python/huffman.py decode ./test_out/unique.huff ./test_out/unique.huff.txt



