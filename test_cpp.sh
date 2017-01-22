#! /bin/bash

./cpp/huff encode ./test_cases/cn.txt ./test_out/cn.huff
./cpp/huff encode ./test_cases/empty.txt ./test_out/empty.huff
./cpp/huff encode ./test_cases/graph.txt ./test_out/graph.huff
./cpp/huff encode ./test_cases/text.txt ./test_out/text.huff
./cpp/huff encode ./test_cases/unique.txt ./test_out/unique.huff

./cpp/huff decode ./test_out/cn.huff ./test_out/cn.huff.txt
./cpp/huff decode ./test_out/empty.huff ./test_out/empty.huff.txt
./cpp/huff decode ./test_out/graph.huff ./test_out/graph.huff.txt
./cpp/huff decode ./test_out/text.huff ./test_out/text.huff.txt
./cpp/huff decode ./test_out/unique.huff ./test_out/unique.huff.txt



